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
#pragma once // VideoSourceImpl.h
#include "AsyncMediaSourceImpl.h"
#include <memory>
#include <unordered_map>

namespace LiveKitCpp
{

class VideoSinkBroadcast;

class AsyncVideoSourceImpl : public AsyncMediaSourceImpl
{
    using Broadcasters = std::unordered_map<rtc::VideoSinkInterface<webrtc::VideoFrame>*,
                                            std::unique_ptr<VideoSinkBroadcast>>;
public:
    ~AsyncVideoSourceImpl() override;
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
    bool stats(webrtc::VideoTrackSourceInterface::Stats& s) const;
    bool stats(int& inputWidth, int& inputHeight) const;
    uint16_t lastFrameId() const noexcept { return _lastFrameId; }
    webrtc::VideoTrackInterface::ContentHint contentHint() const;
    void setContentHint(webrtc::VideoTrackInterface::ContentHint hint);
    // return true if need to request capturer
    bool addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants);
    // return true if need to reset capturer
    bool removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink);
protected:
    AsyncVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                         const std::shared_ptr<Bricks::Logger>& logger = {},
                         bool liveImmediately = false);
    bool frameWanted() const;
    void broadcast(const webrtc::VideoFrame& frame, bool updateStats = true);
    void discard();
    virtual void onContentHintChanged(webrtc::VideoTrackInterface::ContentHint /*hint*/) {}
    // impl. of MediaSourceImpl
    void onClosed() override;
    void onMuted() override;
private:
    Bricks::SafeObj<Broadcasters> _broadcasters;
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic<uint16_t> _lastFrameId = 0U;
    std::atomic<webrtc::VideoTrackInterface::ContentHint> _contentHint;
};

} // namespace LiveKitCpp
