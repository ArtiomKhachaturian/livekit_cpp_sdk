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
#include "VideoSourceImpl.h"
#include "VideoSinkBroadcast.h"
#include "Utils.h"

namespace LiveKitCpp
{

VideoSourceImpl::VideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                 const std::shared_ptr<Bricks::Logger>& logger,
                                 bool liveImmediately)
    : AsyncMediaSourceImpl(std::move(signalingQueue), logger, liveImmediately)
{
}

VideoSourceImpl::~VideoSourceImpl()
{
}

void VideoSourceImpl::processConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            it->second->OnConstraintsChanged(c);
        }
    }
}

bool VideoSourceImpl::stats(webrtc::VideoTrackSourceInterface::Stats& s) const
{
    return stats(s.input_width, s.input_height);
}

bool VideoSourceImpl::stats(int& inputWidth, int& inputHeight) const
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

bool VideoSourceImpl::addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
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

bool VideoSourceImpl::removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        if (_broadcasters->erase(sink) > 0U) {
            return _broadcasters->empty();
        }
    }
    return false;
}

bool VideoSourceImpl::frameWanted() const
{
    if (enabled() && active()) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        return !_broadcasters->empty();
    }
    return false;
}

void VideoSourceImpl::broadcast(const webrtc::VideoFrame& frame, bool updateStats)
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

void VideoSourceImpl::discard()
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnDiscardedFrame();
    }
}

void VideoSourceImpl::onClosed()
{
    AsyncMediaSourceImpl::onClosed();
    _broadcasters({});
}

void VideoSourceImpl::onMuted()
{
    AsyncMediaSourceImpl::onMuted();
    _lastResolution = 0ULL;
    _lastFrameId = 0U;
}

} // namespace LiveKitCpp
