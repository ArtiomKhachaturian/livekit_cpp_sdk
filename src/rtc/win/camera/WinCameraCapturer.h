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
#pragma once
#ifdef WEBRTC_WIN
#include "CameraCapturer.h"
#include "CapturedFrameReceiver.h"
#include "Loggable.h"
#include "SafeObjAliases.h"
#include "Listener.h"
#include <atlbase.h> //CComPtr support
#include <modules/video_capture/windows/device_info_ds.h>
#include <optional>

namespace LiveKitCpp 
{

class CaptureSinkFilter;
class DVCameraConfig;
enum class CapturerState;

class WinCameraCapturer : public Bricks::LoggableS<CameraCapturer>,
                          private CapturedFrameReceiver
{
    using DeviceInfoDS = webrtc::videocapturemodule::DeviceInfoDS;
public:
    ~WinCameraCapturer() override;
    static ::rtc::scoped_refptr<CameraCapturer> 
        create(const MediaDeviceInfo& device, VideoFrameBufferPool framesPool = {},
               const std::shared_ptr<Bricks::Logger>& logger = {});
    // impl. of CameraCapturer
    void setObserver(CapturerObserver* observer) final;
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    bool CaptureStarted() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
protected:
    WinCameraCapturer(const MediaDeviceInfo& device,
                      std::unique_ptr<DeviceInfoDS> deviceInfo,
                      const CComPtr<IBaseFilter>& captureFilter,
                      const CComPtr<IGraphBuilder>& graphBuilder,
                      const CComPtr<IMediaControl>& mediaControl,
                      const CComPtr<IPin>& outputCapturePin,
                      VideoFrameBufferPool framesPool,
                      const std::shared_ptr<Bricks::Logger>& logger);
private:
    static bool connectDVCamera(const CComPtr<IGraphBuilder>& graphBuilder,
                                const CComPtr<IPin>& inputSendPin,
                                const CComPtr<IPin>& outputCapturePin,
                                std::unique_ptr<DVCameraConfig>& outputConfig,
                                const std::shared_ptr<Bricks::Logger>& logger);
    static CComPtr<IPin> findInputSendPin(const CComPtr<IGraphBuilder>& graphBuilder,
                                          IBaseFilter* filter,
                                          const std::shared_ptr<Bricks::Logger>& logger);
    static CComPtr<IPin> findInputPin(IBaseFilter* filter,
                                      const std::shared_ptr<Bricks::Logger>& logger);
    static CComPtr<IPin> findOutputPin(IBaseFilter* filter,
                                       const std::shared_ptr<Bricks::Logger>& logger);
    static CComPtr<IPin> findPin(IBaseFilter* filter,
                                 const std::shared_ptr<Bricks::Logger>& logger,
                                 PIN_DIRECTION expectedDir, 
                                 REFGUID category = GUID_NULL);
    void setCameraState(CapturerState state);
    bool setCameraOutput(const webrtc::VideoCaptureCapability& requestedCapability);
    void disconnect();
    // impl. of CapturedFrameReceiver
    void deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                      DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                      const webrtc::VideoCaptureCapability& frameInfo) final;
private:
    const CComPtr<CaptureSinkFilter> _sinkFilter;
    const std::unique_ptr<DeviceInfoDS> _deviceInfo;
    const CComPtr<IBaseFilter> _captureFilter;
    const CComPtr<IGraphBuilder> _graphBuilder;
    const CComPtr<IMediaControl> _mediaControl;
    const CComPtr<IPin> _outputCapturePin;
    const CComPtr<IPin> _inputSendPin;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _requestedCapability;
    Bricks::SafeUniquePtr<DVCameraConfig> _dvCameraConfig;
    Bricks::Listener<CapturerObserver*> _observer;
};

} // namespace LiveKitCpp
#endif
