// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "AsyncVideoSourceImpl.h"
#include "VideoSinkBroadcast.h"
#include "VideoFrameBufferPoolSource.h"
#include "VideoUtils.h"
#include "VideoFrameImpl.h"
#include "Utils.h"
#include "livekit/rtc/media/LocalVideoFilterPin.h"

namespace  {

inline bool operator == (const std::optional<webrtc::VideoSinkWants::Aggregates>& a,
                         const std::optional<webrtc::VideoSinkWants::Aggregates>& b) {
    return (!a && !b) || a.value() == b.value();
}

inline bool operator != (const webrtc::VideoSinkWants::Aggregates& a,
                         const webrtc::VideoSinkWants::Aggregates& b) {
    return a.any_active_without_requested_resolution != b.any_active_without_requested_resolution;
}

inline bool isDefaultWants(const webrtc::VideoSinkWants& wants) {
    static const webrtc::VideoSinkWants defaultWants;
    return defaultWants.rotation_applied == wants.rotation_applied &&
           defaultWants.black_frames == wants.black_frames &&
           defaultWants.max_pixel_count == wants.max_pixel_count &&
           defaultWants.target_pixel_count == wants.target_pixel_count &&
           defaultWants.max_framerate_fps == wants.max_framerate_fps &&
           defaultWants.resolution_alignment == wants.resolution_alignment &&
           defaultWants.requested_resolution == wants.requested_resolution &&
           defaultWants.is_active == wants.is_active &&
           defaultWants.aggregates == wants.aggregates;
}

}

namespace LiveKitCpp
{

AsyncVideoSourceImpl::AsyncVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                           const std::shared_ptr<Bricks::Logger>& logger,
                                           VideoContentHint initialContentHint, bool liveImmediately)
    : AsyncMediaSourceImpl(std::move(signalingQueue), logger, liveImmediately)
    , _framesPool(VideoFrameBufferPoolSource::create())
    , _contentHint(initialContentHint)
{
#ifdef WEBRTC_MAC
    static_assert(64U == sizeof(webrtc::VideoSinkWants), "check changes in rtc::VideoSinkWants structure");
#elif defined(WEBRTC_WIN)
    // TODO: add assertion
#endif
    if (_framesPool) {
        _framesPool->resize(webrtc::videocapturemodule::kDefaultFrameRate);
    }
}

AsyncVideoSourceImpl::~AsyncVideoSourceImpl()
{
    setFilter(nullptr);
}

void AsyncVideoSourceImpl::setFilter(LocalVideoFilterPin* inputPin)
{
    LOCK_WRITE_SAFE_OBJ(_externalFilter);
    if (_externalFilter != inputPin) {
        if (const auto filter = _externalFilter.constRef()) {
            filter->setReceiver(nullptr);
        }
        _externalFilter = inputPin;
        if (inputPin) {
            inputPin->setReceiver(this);
        }
    }
}

void AsyncVideoSourceImpl::processConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            if (it->second) {
                it->second->OnConstraintsChanged(c);
            }
            else {
                it->first->OnConstraintsChanged(c);
            }
        }
    }
}

bool AsyncVideoSourceImpl::stats(webrtc::VideoTrackSourceInterface::Stats& s) const
{
    return stats(s.input_width, s.input_height);
}

bool AsyncVideoSourceImpl::stats(int& inputWidth, int& inputHeight) const
{
    if (active()) {
        if (const auto resolution = _lastResolution.load()) {
            inputWidth = extractHiWord(resolution);
            inputHeight = extractLoWord(resolution);
            return true;
        }
    }
    return false;
}

bool AsyncVideoSourceImpl::changeContentHint(VideoContentHint hint)
{
    if (active() && exchangeVal(hint, _contentHint)) {
        notifyAboutChanges();
        return true;
    }
    return false;
}

bool AsyncVideoSourceImpl::addOrUpdateSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                           const webrtc::VideoSinkWants& wants)
{
    if (sink && active()) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        const auto it = _broadcasters->find(sink);
        if (it != _broadcasters->end()) {
            if (isDefaultWants(wants)) {
                it->second.reset();
            }
            else {
                if (!it->second) {
                    it->second = std::make_unique<VideoSinkBroadcast>(sink, wants);
                    it->second->setContentHint(contentHint());
                }
                else {
                    it->second->updateSinkWants(wants);
                }
            }
        }
        else {
            std::unique_ptr<VideoSinkBroadcast> adapter;
            if (!isDefaultWants(wants)) {
                adapter = std::make_unique<VideoSinkBroadcast>(sink, wants);
                adapter->setContentHint(contentHint());
            }
            _broadcasters->insert(std::make_pair(sink, std::move(adapter)));
            return 1U == _broadcasters->size();
        }
    }
    return false;
}

bool AsyncVideoSourceImpl::removeSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        if (_broadcasters->erase(sink) > 0U) {
            return _broadcasters->empty();
        }
    }
    return false;
}

void AsyncVideoSourceImpl::updateAfterContentHintChanges(VideoContentHint hint)
{
    if (active()) {
        if (_framesPool) {
            _framesPool->setContentHint(hint);
        }
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            if (it->second) {
                it->second->setContentHint(hint);
            }
        }
    }
}

void AsyncVideoSourceImpl::updateAfterEnableChanges(bool enabled)
{
    AsyncMediaSourceImpl::updateAfterEnableChanges(enabled);
    if (enabled) {
        requestCapturer();
    }
    else {
        resetCapturer();
        if (_framesPool) {
            _framesPool->release();
        }
    }
}

void AsyncVideoSourceImpl::setDeviceInfo(MediaDeviceInfo info)
{
    if (active()) {
        info = validate(info);
        if (info) {
            if (exchangeVal(info, _deviceInfo)) {
                notify(&MediaDeviceListener::onMediaChanged);
                onDeviceInfoChanged(info);
            }
        }
        else {
            // TODO: report about error
        }
    }
}

void AsyncVideoSourceImpl::setOptions(VideoOptions options)
{
    if (active()) {
        options = validate(options);
        if (exchangeVal(options, _options)) {
            notify(&MediaDeviceListener::onMediaOptionsChanged);
            onOptionsChanged(options);
        }
    }
}

bool AsyncVideoSourceImpl::frameWanted() const
{
    if (enabled() && active()) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        return !_broadcasters->empty();
    }
    return false;
}

VideoFrameBufferPool AsyncVideoSourceImpl::framesPool() const
{
    return {_framesPool};
}

void AsyncVideoSourceImpl::onDeviceInfoChanged(const MediaDeviceInfo&)
{
    resetCapturer();
    resetStats();
    requestCapturer();
}

void AsyncVideoSourceImpl::onClosed()
{
    AsyncMediaSourceImpl::onClosed();
    resetCapturer();
    resetStats();
    _broadcasters({});
}

void AsyncVideoSourceImpl::onMuted()
{
    AsyncMediaSourceImpl::onMuted();
    resetStats();
    if (_framesPool) {
        _framesPool->release();
    }
}

void AsyncVideoSourceImpl::onStateChanged(CapturerState state)
{
    switch (state) {
        case CapturerState::Stopped:
            changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
            _framesPool->release();
            break;
        case CapturerState::Starting:
            changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
            break;
        case CapturerState::Started:
            changeState(webrtc::MediaSourceInterface::SourceState::kLive);
            break;
        default:
            break;
    }
}

void AsyncVideoSourceImpl::onCapturingError(std::string details, bool fatal)
{
    if (fatal) {
        changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
        notify(&MediaDeviceListener::onMediaFatalError, details);
    }
}

void AsyncVideoSourceImpl::OnFrame(const webrtc::VideoFrame& frame)
{
    if (active() && frame.video_frame_buffer()) {
        if (!broadcastToFilter(frame)) {
            broadcast(frame);
        }
    }
}

void AsyncVideoSourceImpl::OnDiscardedFrame()
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        if (it->second) {
            it->second->OnDiscardedFrame();
        }
        else {
            it->first->OnDiscardedFrame();
        }
    }
}

void AsyncVideoSourceImpl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        processConstraints(c);
    }
}

void AsyncVideoSourceImpl::broadcast(const webrtc::VideoFrame& frame)
{
    _lastResolution = clueToUint64(frame.width(), frame.height());
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        if (it->second) {
            it->second->OnFrame(frame);
        }
        else {
            it->first->OnFrame(frame);
        }
    }
}

bool AsyncVideoSourceImpl::broadcastToFilter(const webrtc::VideoFrame& frame) const
{
    LOCK_READ_SAFE_OBJ(_externalFilter);
    const auto filter = _externalFilter.constRef();
    if (filter && !filter->paused()) {
        if (const auto frameImpl = VideoFrameImpl::create(frame)) {
            filter->onFrame(frameImpl);
            return true;
        }
    }
    return false;
}

void AsyncVideoSourceImpl::onFrame(const std::shared_ptr<VideoFrame>& frame)
{
    if (active() && enabled()) {
        if (const auto rtcFrame = VideoFrameImpl::create(frame, framesPool())) {
            broadcast(rtcFrame.value());
        }
    }
}

} // namespace LiveKitCpp
