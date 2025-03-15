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
#include "WinCameraCapturer.h"
#include "CameraObserver.h"
#include "CaptureSinkFilter.h"
#include "MediaAuthorization.h"
#include "Utils.h"
//#include "VideoUtils.h"
#include "Windows/MFMediaSampleBuffer.h"
#include <api/media_stream_interface.h>
#include <dvdmedia.h>
#include <modules/video_capture/video_capture_factory.h> //VideoCaptureFactory
#include <modules/video_capture/windows/help_functions_ds.h>
#include <rtc_base/ref_counted_object.h>

#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME L"SinkFilter"

namespace LiveKitCpp 
{

// Microsoft DV interface (external DV cameras)
class WinCameraCapturer::DVCameraConfig
{
public:
    DVCameraConfig(const CComPtr<IGraphBuilder>& graphBuilder,
                   const CComPtr<IPin>& inputDvPin,
                   const CComPtr<IPin>& outputDvPin,
                   const CComPtr<IBaseFilter>& dvFilter,
                   const CComPtr<IPin>& inputSendPin,
                   const CComPtr<IPin>& outputCapturePin);
    ~DVCameraConfig();
    bool connect();
    void disconnect();
private:
    const CComPtr<IGraphBuilder> _graphBuilder;
    const CComPtr<IPin> _inputDvPin;
    const CComPtr<IPin> _outputDvPin;
    const CComPtr<IBaseFilter> _dvFilter;
    const CComPtr<IPin> _inputSendPin;
    const CComPtr<IPin> _outputCapturePin;
};

WinCameraCapturer::WinCameraCapturer(std::string_view deviceGuid,
                                     std::unique_ptr<DeviceInfoDS> deviceInfo,
                                     const CComPtr<IBaseFilter>& captureFilter,
                                     const CComPtr<IGraphBuilder>& graphBuilder,
                                     const CComPtr<IMediaControl>& mediaControl,
                                     const CComPtr<IPin>& outputCapturePin)
    : CameraCapturer(deviceGuid.data())
    , _sinkFilter(new CaptureSinkFilter(this))
    , _deviceInfo(std::move(deviceInfo))
    , _captureFilter(captureFilter)
    , _graphBuilder(graphBuilder)
    , _mediaControl(mediaControl)
    , _outputCapturePin(outputCapturePin)
    , _inputSendPin(findInputSendPin(graphBuilder, _sinkFilter))
    , _observer(nullptr)
{
    assert(_deviceInfo);
    assert(_captureFilter);
    assert(_graphBuilder);
    assert(_mediaControl);
    assert(_outputCapturePin);
}

WinCameraCapturer::~WinCameraCapturer()
{
    _mediaControl->Stop();
    disconnect();
    _graphBuilder->RemoveFilter(_sinkFilter);
    _graphBuilder->RemoveFilter(_captureFilter);
}

::rtc::scoped_refptr<CameraCapturer> WinCameraCapturer::create(std::string_view deviceGuid)
{
    if (!deviceGuid.empty() &&
        MediaAuthorization::maybeAuthorized(MediaAuthorizationKind::Camera, false)) {
        std::unique_ptr<DeviceInfoDS> deviceInfo(DeviceInfoDS::Create());
        if (deviceInfo) {
            const CComPtr<IBaseFilter> captureFilter = deviceInfo->GetDeviceFilter(deviceGuid.data());
            if (captureFilter) {
                CComPtr<IGraphBuilder> graphBuilder;
                HRESULT hr = graphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
                if (COM_IS_OK(hr)) {
                    CComPtr<IMediaControl> mediaControl;
                    hr = graphBuilder->QueryInterface(&mediaControl);
                    if (COM_IS_OK(hr)) {
                        hr = graphBuilder->AddFilter(captureFilter, CAPTURE_FILTER_NAME);
                        if (COM_IS_OK(hr)) {
                            const CComPtr<IPin> outputCapturePin = findOutputPin(captureFilter);
                            if (outputCapturePin) {
                                const auto capturer = ::rtc::make_ref_counted<WinCameraCapturer>(std::move(deviceGuid),
                                                                                                 std::move(deviceInfo),
                                                                                                 captureFilter,
                                                                                                 graphBuilder,
                                                                                                 mediaControl,
                                                                                                 outputCapturePin);
                                if (capturer->_inputSendPin &&
                                    capturer->setCameraOutput(defaultCapability())) {
                                    hr = capturer->_mediaControl->Pause();
                                    if (COM_IS_OK(hr)) {
                                        LOCK_WRITE_SAFE_OBJ(capturer->_requestedCapability);
                                        capturer->_requestedCapability = webrtc::VideoCaptureCapability();
                                        return capturer;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                Logger::logw("failed to create capture filter");
            }
        } else {
            Logger::logw("failed to create DS info module");
        }
    }
    return nullptr;
}

void WinCameraCapturer::setObserver(CameraObserver* observer)
{
    _observer = observer;
}

int32_t WinCameraCapturer::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    LOCK_READ_SAFE_OBJ(_requestedCapability);
    if (_requestedCapability.constRef() != capability) {
        const bool stopped = COM_IS_OK(_mediaControl->Stop());
        bool started = false;
        if (stopped) {
            disconnect();
            if (setCameraOutput(capability)) {
                started = COM_IS_OK(_mediaControl->Run());
            }
        }
        if (started) {
            setCameraState(CameraState::Started);
        } else if (stopped) {
            setCameraState(CameraState::Stopped);
        }
        if (!started) {
            return -1;
        }
    }
    return 0;
}

int32_t WinCameraCapturer::StopCapture()
{
    if (COM_IS_OK(_mediaControl->Pause())) {
        setCameraState(CameraState::Stopped);
        return 0;
    }
    return -1;
}

bool WinCameraCapturer::CaptureStarted()
{
    OAFilterState state = 0;
    _mediaControl->GetState(1000, &state);
    return state == State_Running;
}

int32_t WinCameraCapturer::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    LOCK_READ_SAFE_OBJ(_requestedCapability);
    settings = _requestedCapability.constRef();
    return 0;
}

webrtc::VideoCaptureCapability WinCameraCapturer::defaultCapability()
{
    webrtc::VideoCaptureCapability caps;
    caps.width = webrtc::videocapturemodule::kDefaultWidth;
    caps.height = webrtc::videocapturemodule::kDefaultHeight;
    caps.maxFPS = webrtc::videocapturemodule::kDefaultFrameRate;
    caps.videoType = webrtc::VideoType::kI420;
    return caps;
}

void WinCameraCapturer::deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                                     DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                                     const webrtc::VideoCaptureCapability& frameInfo)
{
    if (buffer && actualBufferLen > 0UL && hasCaptureDataCallback()) {
        assert(totalBufferLen >= actualBufferLen);
        const auto videoBuffer = MFMediaSampleBuffer::create(frameInfo, buffer,
                                                             actualBufferLen, totalBufferLen, sample,
                                                             std::nullopt, i420BuffersPool(), captureRotation());
        if (videoBuffer) {
            const auto videoFrame = createVideoFrame(videoBuffer);
            if (videoFrame.has_value()) {
                sendFrame(videoFrame.value());
            }
        } else {
            Logger::logw("failed to create captured video buffer from type " + std::to_string(static_cast<int>(frameInfo.videoType)));
            discardFrame();
        }
    }
}

bool WinCameraCapturer::connectDVCamera(const CComPtr<IGraphBuilder>& graphBuilder,
                                        const CComPtr<IPin>& inputSendPin,
                                        const CComPtr<IPin>& outputCapturePin,
                                        std::unique_ptr<DVCameraConfig>& outputConfig)
{
    if (graphBuilder && inputSendPin && outputCapturePin) {
        CComPtr<IBaseFilter> dvFilter;
        HRESULT hr = dvFilter.CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC);
        if (COM_IS_OK(hr)) {
            hr = graphBuilder->AddFilter(dvFilter, L"VideoDecoderDV");
            if (COM_IS_OK(hr)) {
                const CComPtr<IPin> inputDvPin = findInputPin(dvFilter);
                if (inputDvPin) {
                    const CComPtr<IPin> outputDvPin = findOutputPin(dvFilter);
                    if (outputDvPin) {
                        auto newConfig = std::make_unique<DVCameraConfig>(graphBuilder, inputDvPin,
                                                                          outputDvPin, dvFilter,
                                                                          inputSendPin, outputCapturePin);
                        if (newConfig->connect()) {
                            outputConfig = std::move(newConfig);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

CComPtr<IPin> WinCameraCapturer::findInputSendPin(const CComPtr<IGraphBuilder>& graphBuilder,
                                                  IBaseFilter* filter)
{
    if (graphBuilder && filter) {
        if (COM_IS_OK(graphBuilder->AddFilter(filter, SINK_FILTER_NAME))) {
            return findInputPin(filter);
        }
    }
    return CComPtr<IPin>();
}

CComPtr<IPin> WinCameraCapturer::findInputPin(IBaseFilter* filter)
{
    return findPin(filter, PINDIR_INPUT);
}

CComPtr<IPin> WinCameraCapturer::findOutputPin(IBaseFilter* filter)
{
    return findPin(filter, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE);
}

CComPtr<IPin> WinCameraCapturer::findPin(IBaseFilter* filter,
                                         PIN_DIRECTION expectedDir, REFGUID category)
{
    if (filter) {
        CComPtr<IEnumPins> pinEnums;
        HRESULT hr = filter->EnumPins(&pinEnums);
        if (COM_IS_OK(hr)) {
            hr = pinEnums->Reset();
            if (COM_IS_OK(hr)) {
                CComPtr<IPin> pin;
                while (S_OK == pinEnums->Next(1UL, &pin, NULL)) {
                    PIN_DIRECTION pinDir;
                    hr = pin->QueryDirection(&pinDir);
                    if (COM_IS_OK(hr)) {
                        bool accepted = expectedDir == pinDir; // this is an expected pin
                        if (accepted && GUID_NULL != category) {
                            accepted = webrtc::videocapturemodule::PinMatchesCategory(pin, category);
                        }
                        if (accepted && PINDIR_INPUT == pinDir) {
                            CComPtr<IPin> tempPin;
                            hr = pin->ConnectedTo(&tempPin);
                            accepted = FAILED(hr); // the pin is not connected
                        }
                        if (accepted) {
                            return pin;
                        }
                    }
                    pin.Release();
                }
            }
        }
    }
    return CComPtr<IPin>();
}

void WinCameraCapturer::setCameraState(CameraState state)
{
    _observer.invoke(&CameraObserver::onStateChanged, state);
}

bool WinCameraCapturer::setCameraOutput(const webrtc::VideoCaptureCapability& requestedCapability)
{
    bool ok = false;
    if (_inputSendPin) {
        // get the best matching capability
        webrtc::VideoCaptureCapability capability;
        // match the requested capability with the supported
        const int32_t capabilityIndex = _deviceInfo->GetBestMatchedCapability(CurrentDeviceName(), requestedCapability, capability);
        if (capabilityIndex >= 0) {
            // reduce the frame rate if possible.
            if (capability.maxFPS > requestedCapability.maxFPS) {
                capability.maxFPS = requestedCapability.maxFPS;
            } else if (capability.maxFPS <= 0) {
                capability.maxFPS = 30;
            }
            // convert it to the windows capability index since they are not neccessary the same
            webrtc::videocapturemodule::VideoCaptureCapabilityWindows windowsCapability;
            if (0 == _deviceInfo->GetWindowsCapability(capabilityIndex, windowsCapability)) {
                CComPtr<IAMStreamConfig> streamConfig;
                HRESULT hr = _outputCapturePin->QueryInterface(&streamConfig);
                if (COM_IS_OK(hr)) {
                    AM_MEDIA_TYPE* pmt = NULL;
                    VIDEO_STREAM_CONFIG_CAPS caps;
                    // get the windows capability from the capture device
                    hr = streamConfig->GetStreamCaps(windowsCapability.directShowCapabilityIndex,
                                                     &pmt, reinterpret_cast<BYTE*>(&caps));
                    if (COM_IS_OK(hr)) {
                        if (pmt->formattype == FORMAT_VideoInfo2) {
                            VIDEOINFOHEADER2* h = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->pbFormat);
                            if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
                                h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
                            }
                        } else {
                            VIDEOINFOHEADER* h = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
                            if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
                                h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
                            }
                        }
                        // Set the sink filter to request this capability
                        hr = _sinkFilter->setRequestedCapability(capability);
                        if (COM_IS_OK(hr)) {
                            // order the capture device to use this capability
                            hr = streamConfig->SetFormat(pmt);
                            if (COM_IS_OK(hr)) {
                                bool connected = false;
                                const bool isDVCamera = pmt->subtype == MEDIASUBTYPE_dvsl ||
                                                        pmt->subtype == MEDIASUBTYPE_dvsd ||
                                                        pmt->subtype == MEDIASUBTYPE_dvhd;
                                if (isDVCamera) {
                                    LOCK_WRITE_SAFE_OBJ(_dvCameraConfig);
                                    if (!_dvCameraConfig.constRef()) {
                                        connected = connectDVCamera(_graphBuilder,
                                                                    _inputSendPin, _outputCapturePin,
                                                                    _dvCameraConfig.ref());
                                    } else {
                                        connected = _dvCameraConfig.constRef()->connect();
                                    }

                                } else {
                                    connected = COM_IS_OK(_graphBuilder->ConnectDirect(_outputCapturePin,
                                                                                       _inputSendPin, NULL));
                                }
                                if (connected) {
                                    LOCK_WRITE_SAFE_OBJ(_requestedCapability);
                                    _requestedCapability = capability;
                                    ok = true;
                                }
                            }
                        }
                        webrtc::videocapturemodule::FreeMediaType(pmt);
                    }
                }
            }
        }
    }
    return ok;
}

void WinCameraCapturer::disconnect()
{
    TRACE_COM_ERROR(_graphBuilder->Disconnect(_outputCapturePin));
    if (_inputSendPin) {
        TRACE_COM_ERROR(_graphBuilder->Disconnect(_inputSendPin));
    }
    LOCK_READ_SAFE_OBJ(_dvCameraConfig);
    if (const auto& dvCameraConfig = _dvCameraConfig.constRef()) {
        dvCameraConfig->disconnect();
    }
}

WinCameraCapturer::DVCameraConfig::DVCameraConfig(const CComPtr<IGraphBuilder>& graphBuilder,
                                                  const CComPtr<IPin>& inputDvPin,
                                                  const CComPtr<IPin>& outputDvPin,
                                                  const CComPtr<IBaseFilter>& dvFilter,
                                                  const CComPtr<IPin>& inputSendPin,
                                                  const CComPtr<IPin>& outputCapturePin)
    : _graphBuilder(graphBuilder)
    , _inputDvPin(inputDvPin)
    , _outputDvPin(outputDvPin)
    , _dvFilter(dvFilter)
    , _inputSendPin(inputSendPin)
    , _outputCapturePin(outputCapturePin)
{
    assert(_graphBuilder);
    assert(_inputDvPin);
    assert(_outputDvPin);
    assert(_dvFilter);
    assert(_inputSendPin);
    assert(_outputCapturePin);
}

WinCameraCapturer::DVCameraConfig::~DVCameraConfig()
{
    _graphBuilder->RemoveFilter(_dvFilter);
}

bool WinCameraCapturer::DVCameraConfig::connect()
{
    if (COM_IS_OK(_graphBuilder->ConnectDirect(_outputCapturePin, _inputDvPin, NULL))) {
        return COM_IS_OK(_graphBuilder->ConnectDirect(_outputDvPin, _inputSendPin, NULL));
    }
    return false;
}

void WinCameraCapturer::DVCameraConfig::disconnect()
{
    TRACE_COM_ERROR(_graphBuilder->Disconnect(_inputDvPin));
    TRACE_COM_ERROR(_graphBuilder->Disconnect(_outputDvPin));
}

} // namespace LiveKitCpp