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
#include "Loggable.h"
#include "CameraCapturer.h"
#include "CapturedFrameReceiver.h"
#include "SafeObjAliases.h"
#include "Listener.h"
#include <atlbase.h> //CComPtr support
#include <modules/video_capture/windows/device_info_ds.h>
#include <optional>

namespace LiveKitCpp 
{

class CaptureSinkFilter;
enum class CameraState;

class WinCameraCapturer : public Bricks::LoggableS<CameraCapturer>,
                          private CapturedFrameReceiver
{
    class DVCameraConfig;
    using DeviceInfoDS = webrtc::videocapturemodule::DeviceInfoDS;
public:
    ~WinCameraCapturer() override;
    static ::rtc::scoped_refptr<CameraCapturer> 
        create(std::string_view guid, const std::shared_ptr<Bricks::Logger>& logger = {});
    // impl. of CameraCapturer
    void setObserver(CameraObserver* observer) final;
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    bool CaptureStarted() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
    const char* CurrentDeviceName() const final;
protected:
    WinCameraCapturer(std::string_view guid,
                      std::unique_ptr<DeviceInfoDS> deviceInfo,
                      const CComPtr<IBaseFilter>& captureFilter,
                      const CComPtr<IGraphBuilder>& graphBuilder,
                      const CComPtr<IMediaControl>& mediaControl,
                      const CComPtr<IPin>& outputCapturePin,
                      const std::shared_ptr<Bricks::Logger>& logger);
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final;
private:
    static webrtc::VideoCaptureCapability defaultCapability();
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
    void setCameraState(CameraState state);
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
    Bricks::Listener<CameraObserver*> _observer;
};

} // namespace LiveKitCpp