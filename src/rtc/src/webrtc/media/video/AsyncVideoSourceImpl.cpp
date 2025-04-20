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
#include "Utils.h"

namespace  {

inline bool operator == (const std::optional<rtc::VideoSinkWants::Aggregates>& a,
                         const std::optional<rtc::VideoSinkWants::Aggregates>& b) {
    return (!a && !b) || a.value() == b.value();
}

inline bool operator != (const rtc::VideoSinkWants::Aggregates& a,
                         const rtc::VideoSinkWants::Aggregates& b) {
    return a.any_active_without_requested_resolution != b.any_active_without_requested_resolution;
}

inline bool isDefaultWants(const rtc::VideoSinkWants& wants) {
    static const rtc::VideoSinkWants defaultWants;
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
                                           bool liveImmediately)
    : AsyncMediaSourceImpl(std::move(signalingQueue), logger, liveImmediately)
{
    static_assert(64U == sizeof(rtc::VideoSinkWants), "check changes in rtc::VideoSinkWants structure");
}

AsyncVideoSourceImpl::~AsyncVideoSourceImpl()
{
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

void AsyncVideoSourceImpl::setContentHint(VideoContentHint hint)
{
    if (active() && exchangeVal(hint, _contentHint)) {
        onContentHintChanged(hint);
    }
}

bool AsyncVideoSourceImpl::addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                           const rtc::VideoSinkWants& wants)
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
            }
            _broadcasters->insert(std::make_pair(sink, std::move(adapter)));
            return 1U == _broadcasters->size();
        }
    }
    return false;
}

bool AsyncVideoSourceImpl::removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        if (_broadcasters->erase(sink) > 0U) {
            return _broadcasters->empty();
        }
    }
    return false;
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
        if (options && exchangeVal(options, _options)) {
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

void AsyncVideoSourceImpl::broadcast(const webrtc::VideoFrame& frame, bool updateStats)
{
    if (active() && frame.video_frame_buffer()) {
        if (updateStats) {
            _lastResolution = clueToUint64(frame.width(), frame.height());
            _lastFrameId = frame.id();
        }
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
}

void AsyncVideoSourceImpl::discard()
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
}

void AsyncVideoSourceImpl::onEnabled(bool enabled)
{
    AsyncMediaSourceImpl::onEnabled(enabled);
    if (enabled) {
        requestCapturer();
    }
    else {
        resetCapturer();
    }
}

void AsyncVideoSourceImpl::onStateChanged(CapturerState state)
{
    switch (state) {
        case CapturerState::Stopped:
            changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
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
    changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
    if (fatal) {
        notify(&MediaDeviceListener::onMediaFatalError, details);
    }
}

void AsyncVideoSourceImpl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        processConstraints(c);
    }
}

void AsyncVideoSourceImpl::resetStats()
{
    _lastResolution = 0ULL;
    _lastFrameId = 0U;
}

} // namespace LiveKitCpp
