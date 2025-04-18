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

namespace LiveKitCpp
{

AsyncVideoSourceImpl::AsyncVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                           const std::shared_ptr<Bricks::Logger>& logger,
                                           bool liveImmediately)
    : AsyncMediaSourceImpl(std::move(signalingQueue), logger, liveImmediately)
{
}

AsyncVideoSourceImpl::~AsyncVideoSourceImpl()
{
}

void AsyncVideoSourceImpl::processConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            it->second->OnConstraintsChanged(c);
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
            it->second->updateSinkWants(wants);
        }
        else {
            auto adapter = std::make_unique<VideoSinkBroadcast>(sink, wants);
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
        if (info && exchangeVal(std::move(info), _deviceInfo)) {
            notify(&MediaDeviceListener::onMediaChanged);
            resetCapturer();
            resetStats();
            requestCapturer();
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
            it->second->OnFrame(frame);
        }
    }
}

void AsyncVideoSourceImpl::discard()
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnDiscardedFrame();
    }
}

void AsyncVideoSourceImpl::onClosed()
{
    AsyncMediaSourceImpl::onClosed();
    resetStats();
    _broadcasters({});
}

void AsyncVideoSourceImpl::onMuted()
{
    AsyncMediaSourceImpl::onMuted();
    resetStats();
}

void AsyncVideoSourceImpl::resetStats()
{
    _lastResolution = 0ULL;
    _lastFrameId = 0U;
}

} // namespace LiveKitCpp
