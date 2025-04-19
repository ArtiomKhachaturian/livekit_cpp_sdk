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
#include "CapturerProxySink.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include "livekit/rtc/media/VideoOptions.h"
#include "livekit/rtc/media/VideoContentHint.h"
#include <memory>
#include <unordered_map>

namespace LiveKitCpp
{

class VideoSinkBroadcast;

class AsyncVideoSourceImpl : public AsyncMediaSourceImpl, protected CapturerProxySink
{
    using Broadcasters = std::unordered_map<rtc::VideoSinkInterface<webrtc::VideoFrame>*,
    std::unique_ptr<VideoSinkBroadcast>>;
public:
    ~AsyncVideoSourceImpl() override;
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
    bool stats(webrtc::VideoTrackSourceInterface::Stats& s) const;
    bool stats(int& inputWidth, int& inputHeight) const;
    uint16_t lastFrameId() const noexcept { return _lastFrameId; }
    VideoContentHint contentHint() const { return _contentHint; }
    void setContentHint(VideoContentHint hint);
    MediaDeviceInfo deviceInfo() const { return _deviceInfo(); }
    void setDeviceInfo(MediaDeviceInfo info);
    void setOptions(VideoOptions options = {});
    VideoOptions options() const { return _options(); }
    // return true if need to request capturer
    bool addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants);
    // return true if need to reset capturer
    bool removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink);
    virtual void requestCapturer() {}
    virtual void resetCapturer() {}
protected:
    AsyncVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                         const std::shared_ptr<Bricks::Logger>& logger = {},
                         bool liveImmediately = false);
    bool frameWanted() const;
    void broadcast(const webrtc::VideoFrame& frame, bool updateStats = true);
    void discard();
    virtual void onContentHintChanged(VideoContentHint /*hint*/) {}
    virtual void onOptionsChanged(const VideoOptions& /*options*/ ) {}
    virtual MediaDeviceInfo validate(MediaDeviceInfo info) const { return info; }
    virtual VideoOptions validate(VideoOptions options) const { return options; }
    // impl. of MediaSourceImpl
    void onClosed() override;
    void onMuted() override;
    // impl. of CapturerObserver
    void onStateChanged(CapturerState state) override;
    void onCapturingFatalError(const std::string& details) override;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) override { broadcast(frame, true); }
    void OnDiscardedFrame() override { discard(); }
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c) override;
private:
    void resetStats();
private:
    Bricks::SafeObj<Broadcasters> _broadcasters;
    Bricks::SafeObj<MediaDeviceInfo> _deviceInfo;
    Bricks::SafeObj<VideoOptions> _options;
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic<uint16_t> _lastFrameId = 0U;
    std::atomic<VideoContentHint> _contentHint = VideoContentHint::None;
};

} // namespace LiveKitCpp
