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
#include "VideoFrameBufferPool.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include "livekit/rtc/media/VideoOptions.h"
#include "livekit/rtc/media/VideoContentHint.h"
#include "livekit/rtc/media/VideoSink.h"
#include <memory>
#include <unordered_map>

namespace LiveKitCpp
{

class VideoSinkBroadcast;
class LocalVideoFilterPin;

class AsyncVideoSourceImpl : public AsyncMediaSourceImpl,
                             protected CapturerProxySink,
                             private VideoSink
{
    using Broadcasters = std::unordered_map<webrtc::VideoSinkInterface<webrtc::VideoFrame>*, std::unique_ptr<VideoSinkBroadcast>>;
public:
    ~AsyncVideoSourceImpl() override;
    void setFilter(LocalVideoFilterPin* inputPin);
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
    bool stats(webrtc::VideoTrackSourceInterface::Stats& s) const;
    bool stats(int& inputWidth, int& inputHeight) const;
    VideoContentHint contentHint() const { return _contentHint; }
    virtual bool changeContentHint(VideoContentHint hint);
    MediaDeviceInfo deviceInfo() const { return _deviceInfo(); }
    void setDeviceInfo(MediaDeviceInfo info);
    void setOptions(VideoOptions options = {});
    VideoOptions options() const { return _options(); }
    // return true if need to request capturer
    bool addOrUpdateSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const webrtc::VideoSinkWants& wants);
    // return true if need to reset capturer
    bool removeSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink);
    virtual void updateAfterContentHintChanges(VideoContentHint hint);
    virtual void requestCapturer() {}
    virtual void resetCapturer() {}
    // override of AsyncMediaSourceImpl
    void updateAfterEnableChanges(bool enabled) override;
protected:
    AsyncVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                         const std::shared_ptr<Bricks::Logger>& logger = {},
                         VideoContentHint initialContentHint = VideoContentHint::None,
                         bool liveImmediately = false);
    bool frameWanted() const;
    // video frames pool
    VideoFrameBufferPool framesPool() const;
    virtual void onOptionsChanged(const VideoOptions& /*options*/ ) {}
    virtual void onDeviceInfoChanged(const MediaDeviceInfo& info);
    virtual MediaDeviceInfo validate(MediaDeviceInfo info) const { return info; }
    virtual VideoOptions validate(VideoOptions options) const { return options; }
    // impl. of MediaSourceImpl
    void onClosed() override;
    void onMuted() override;
    // impl. of CapturerObserver
    void onStateChanged(CapturerState state) override;
    void onCapturingError(std::string details, bool fatal) override;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) override;
    void OnDiscardedFrame() override;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c) override;
private:
    void resetStats() { _lastResolution = 0ULL; }
    void broadcast(const webrtc::VideoFrame& frame);
    bool broadcastToFilter(const webrtc::VideoFrame& frame) const;
    // impl. of VideoSink
    void onFrame(const std::shared_ptr<VideoFrame>& frame) final;
private:
    const std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    Bricks::SafeObj<LocalVideoFilterPin*> _externalFilter = nullptr;
    Bricks::SafeObj<Broadcasters> _broadcasters;
    Bricks::SafeObj<MediaDeviceInfo> _deviceInfo;
    Bricks::SafeObj<VideoOptions> _options;
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic<VideoContentHint> _contentHint;
};

} // namespace LiveKitCpp
