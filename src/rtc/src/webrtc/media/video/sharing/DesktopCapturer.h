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
#pragma once // DesktopCapturer.h
#include "CapturerState.h"
#include "Listener.h"
#include "VideoFrameBufferPool.h"
#include <api/video/video_frame.h>
#include <modules/desktop_capture/desktop_capture_types.h>
#include <modules/desktop_capture/desktop_geometry.h>
#include <modules/desktop_capture/desktop_capture_options.h>
#include <atomic>

namespace webrtc {
class DesktopFrame;
class SharedMemoryFactory;
struct VideoTrackSourceConstraints;
}

namespace LiveKitCpp
{

class CapturerProxySink;

class DesktopCapturer
{
public:
    virtual std::string windowIdToString(webrtc::WindowId id) const;
    virtual std::string screenIdToString(webrtc::ScreenId id) const;
    virtual std::optional<webrtc::WindowId> windowIdFromString(const std::string& str) const;
    virtual std::optional<webrtc::ScreenId> screenIdFromString(const std::string& str) const;
    virtual void updateQualityToContentHint() {}
    // return null opt in case of a failure, title maybe empty for some windows
    virtual std::optional<std::string> title(const std::string& source) const;
    // max expected frame rate for this capturer
    virtual void setTargetFramerate(int32_t /*fps*/) {}
    // upped-bound target resolution, cropped frame should keep original (screen/window) aspect ratio
    virtual void setTargetResolution(int32_t /*width*/, int32_t /*height*/) {}
    // valid only for screen capturers
    virtual webrtc::DesktopSize screenResolution(const std::string& source) const;
    // gets a list of sources current capturer supports, Returns false in case of a failure
    virtual bool enumerateSources(std::vector<std::string>& sources) const;
    // selects a source to be captured, returns false in case of a failure
    // (e.g. if there is no source with the specified type and id.)
    virtual bool selectSource(const std::string& source) = 0;
    // selected source ID if any
    virtual std::string selectedSource() const = 0;
    // tells that capturing is active
    virtual bool started() const = 0;
    // called at the beginning of a capturing session
    virtual bool start() = 0;
    // called at the finish of a capturing session
    virtual void stop() = 0;
    // brings the selected source to the front and sets the input focus on it
    virtual void focusOnSelectedSource() {}
    // sets the window to be excluded from the captured image in the future Capture calls,
    // used to exclude the screenshare notification window (indicator) for screen capturing
    virtual void setExcludedWindow(webrtc::WindowId /*window*/) {}
    void setOutputSink(CapturerProxySink* sink);
    // window or screen capturer
    bool window() const noexcept { return _window; }
    // mode
    bool previewMode() const noexcept { return _previewMode; }
    // overloaded version
    void setTargetResolution(const webrtc::DesktopSize& resolution);
    virtual ~DesktopCapturer() = default;
protected:
    DesktopCapturer(bool window, bool previewMode,
                    webrtc::DesktopCaptureOptions options,
                    VideoFrameBufferPool framesPool = {});
    const auto& options() const noexcept { return _options; }
    bool hasOutputSink() const { return !_sink.empty(); }
    VideoFrameBufferPool framesPool() const noexcept { return _framesPool; }
    CapturerState state() const { return _state; }
    bool changeState(CapturerState state);
    void notifyAboutError(std::string details = {}, bool fatal = true) const;
    void discardFrame();
    void deliverCaptured(const webrtc::VideoFrame& frame);
    void deliverCaptured(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                         int64_t timeStampMicro = 0LL,
                         webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                         const std::optional<webrtc::ColorSpace>& colorSpace = {});
    void deliverCaptured(std::unique_ptr<webrtc::DesktopFrame> frame);
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
private:
    int64_t adjustTimestamp(int64_t timeStampMicro) const;
private:
    const bool _window;
    const bool _previewMode;
    const webrtc::DesktopCaptureOptions _options;
    const VideoFrameBufferPool _framesPool;
    Bricks::Listener<CapturerProxySink*> _sink;
    Bricks::SafeObj<CapturerState> _state = CapturerState::Stopped;
    Bricks::SafeObj<int64_t> _lastTimestamp = 0LL;
};
	
} // namespace LiveKitCpp
