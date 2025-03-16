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
#ifdef WEBRTC_WIN
#include "WinCameraCapturer.h"
#include "CameraManager.h"
#include "CameraObserver.h"
#include "CaptureSinkFilter.h"
#include "CameraErrorHandling.h"
#include "DVCameraConfig.h"
#include "./video/MFMediaSampleBuffer.h"
#include <api/media_stream_interface.h>
#include <dvdmedia.h>
#include <modules/video_capture/video_capture_factory.h> //VideoCaptureFactory
#include <modules/video_capture/windows/help_functions_ds.h>
#include <rtc_base/ref_counted_object.h>

#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME L"SinkFilter"

namespace LiveKitCpp 
{

WinCameraCapturer::WinCameraCapturer(const MediaDevice& device,
                                     std::unique_ptr<DeviceInfoDS> deviceInfo,
                                     const CComPtr<IBaseFilter>& captureFilter,
                                     const CComPtr<IGraphBuilder>& graphBuilder,
                                     const CComPtr<IMediaControl>& mediaControl,
                                     const CComPtr<IPin>& outputCapturePin,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<CameraCapturer>(logger, device)
    , _sinkFilter(new CaptureSinkFilter(this, logger))
    , _deviceInfo(std::move(deviceInfo))
    , _captureFilter(captureFilter)
    , _graphBuilder(graphBuilder)
    , _mediaControl(mediaControl)
    , _outputCapturePin(outputCapturePin)
    , _inputSendPin(findInputSendPin(graphBuilder, _sinkFilter, logger))
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

std::string_view WinCameraCapturer::logCategory() const
{
    return CameraManager::logCategory();
}

::rtc::scoped_refptr<CameraCapturer> WinCameraCapturer::create(const MediaDevice& device,
                                                               const std::shared_ptr<Bricks::Logger>& logger)
{
    const auto& guid = device._guid;
    if (guid.empty()) {
        return {};
    }
    std::unique_ptr<DeviceInfoDS> deviceInfo(DeviceInfoDS::Create());
    if (!deviceInfo) {
        if (logger) {
            logger->logError("failed to create DS info module");
        }
        return {};
    }
    const CComPtr<IBaseFilter> captureFilter = deviceInfo->GetDeviceFilter(guid.data());
    if (!captureFilter) {
        if (logger) {
            logger->logError("failed to create capture filter");
        }
        return {};
    }
    CComPtr<IGraphBuilder> graphBuilder;
    HRESULT hr = graphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    CComPtr<IMediaControl> mediaControl;
    hr = graphBuilder->QueryInterface(&mediaControl);
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    hr = graphBuilder->AddFilter(captureFilter, CAPTURE_FILTER_NAME);
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    const CComPtr<IPin> outputCapturePin = findOutputPin(captureFilter, logger);
    if (!outputCapturePin) {
        return {};
    }
    const auto capturer = rtc::make_ref_counted<WinCameraCapturer>(device,
                                                                   std::move(deviceInfo),
                                                                   captureFilter,
                                                                   graphBuilder,
                                                                   mediaControl,
                                                                   outputCapturePin,
                                                                   logger);
    if (!capturer->_inputSendPin) {
        return {};
    }
    if (!capturer->setCameraOutput(CameraManager::defaultCapability())) {
        return {};
    }
    hr = capturer->_mediaControl->Pause();
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    capturer->_requestedCapability({});
    return capturer;
}

void WinCameraCapturer::setObserver(CameraObserver* observer)
{
    _observer = observer;
}

int32_t WinCameraCapturer::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    LOCK_READ_SAFE_OBJ(_requestedCapability);
    if (_requestedCapability.constRef() != capability) {
        const bool stopped = LOGGABLE_COM_IS_OK(_mediaControl->Stop());
        bool started = false;
        if (stopped) {
            disconnect();
            if (setCameraOutput(capability)) {
                started = LOGGABLE_COM_IS_OK(_mediaControl->Run());
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
    if (LOGGABLE_COM_IS_OK(_mediaControl->Pause())) {
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
    settings = _requestedCapability();
    return 0;
}

void WinCameraCapturer::deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                                     DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                                     const webrtc::VideoCaptureCapability& frameInfo)
{
    if (!buffer || !actualBufferLen || !hasSink()) {
        return;
    }
    assert(totalBufferLen >= actualBufferLen);
    const auto sampleBuffer = MFMediaSampleBuffer::create(frameInfo, buffer,
                                                          actualBufferLen, totalBufferLen, 
                                                          sample, captureRotation());
    if (!sampleBuffer) {
        logWarning("failed to create captured video buffer from type " + 
                   std::to_string(static_cast<int>(frameInfo.videoType)));
        discardFrame();
        return;
    }
    const auto videoFrame = createVideoFrame(sampleBuffer);
    if (videoFrame.has_value()) {
        sendFrame(videoFrame.value());
    }
}

bool WinCameraCapturer::connectDVCamera(const CComPtr<IGraphBuilder>& graphBuilder,
                                        const CComPtr<IPin>& inputSendPin,
                                        const CComPtr<IPin>& outputCapturePin,
                                        std::unique_ptr<DVCameraConfig>& outputConfig,
                                        const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!graphBuilder || !inputSendPin || !outputCapturePin) {
        return false;
    }
    CComPtr<IBaseFilter> dvFilter;
    HRESULT hr = dvFilter.CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC);
    if (!CAMERA_IS_OK(hr, logger)) {
        return false;
    }
    hr = graphBuilder->AddFilter(dvFilter, L"VideoDecoderDV");
    if (!CAMERA_IS_OK(hr, logger)) {
        return false;
    }
    const CComPtr<IPin> inputDvPin = findInputPin(dvFilter, logger);
    if (!inputDvPin) {
        return false;
    }
    const CComPtr<IPin> outputDvPin = findOutputPin(dvFilter, logger);
    if (!outputDvPin) {
        return false;
    }
    auto newConfig = std::make_unique<DVCameraConfig>(graphBuilder, inputDvPin,
                                                      outputDvPin, dvFilter,
                                                      inputSendPin,
                                                      outputCapturePin,
                                                      logger);
    if (!newConfig->connect()) {
        return false;
    }
    outputConfig = std::move(newConfig);
    return true;
}

CComPtr<IPin> WinCameraCapturer::findInputSendPin(const CComPtr<IGraphBuilder>& graphBuilder,
                                                  IBaseFilter* filter,
                                                  const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!graphBuilder || !filter) {
        return {};
    }
    if (!CAMERA_IS_OK(graphBuilder->AddFilter(filter, SINK_FILTER_NAME), logger)) {
        return {};
    }
    return findInputPin(filter, logger);
}

CComPtr<IPin> WinCameraCapturer::findInputPin(IBaseFilter* filter,
                                              const std::shared_ptr<Bricks::Logger>& logger)
{
    return findPin(filter, logger, PINDIR_INPUT);
}

CComPtr<IPin> WinCameraCapturer::findOutputPin(IBaseFilter* filter,
                                               const std::shared_ptr<Bricks::Logger>& logger)
{
    return findPin(filter, logger, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE);
}

CComPtr<IPin> WinCameraCapturer::findPin(IBaseFilter* filter,
                                         const std::shared_ptr<Bricks::Logger>& logger,
                                         PIN_DIRECTION expectedDir, 
                                         REFGUID category)
{
    if (!filter) {
        return {};
    }
    CComPtr<IEnumPins> pinEnums;
    HRESULT hr = filter->EnumPins(&pinEnums);
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    hr = pinEnums->Reset();
    if (!CAMERA_IS_OK(hr, logger)) {
        return {};
    }
    CComPtr<IPin> pin;
    while (S_OK == pinEnums->Next(1UL, &pin, NULL)) {
        PIN_DIRECTION pinDir;
        hr = pin->QueryDirection(&pinDir);
        if (CAMERA_IS_OK(hr, logger)) {
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
    return {};
}

void WinCameraCapturer::setCameraState(CameraState state)
{
    _observer.invoke(&CameraObserver::onStateChanged, state);
}

bool WinCameraCapturer::setCameraOutput(const webrtc::VideoCaptureCapability& requestedCapability)
{
    if (!_inputSendPin) {
        return false;
    }
    // get the best matching capability
    webrtc::VideoCaptureCapability capability;
    // match the requested capability with the supported
    const int32_t capabilityIndex = _deviceInfo->GetBestMatchedCapability(guid().data(), 
                                                                          requestedCapability, 
                                                                          capability);
    if (capabilityIndex < 0) {
        logError("unable to get best matched capability for [" + toString(requestedCapability) + "]");
        return false;
    }
    // reduce the frame rate if possible.
    if (capability.maxFPS > requestedCapability.maxFPS) {
        capability.maxFPS = requestedCapability.maxFPS;
    }
    else if (capability.maxFPS <= 0) {
        capability.maxFPS = 30;
    }
    // convert it to the windows capability index since they are not neccessary the same
    webrtc::videocapturemodule::VideoCaptureCapabilityWindows windowsCapability;
    if (0 != _deviceInfo->GetWindowsCapability(capabilityIndex, windowsCapability)) {
        logError("unable to get windows capability for [" + toString(requestedCapability) + "]");
        return false;
    }
    CComPtr<IAMStreamConfig> streamConfig;
    HRESULT hr = _outputCapturePin->QueryInterface(&streamConfig);
    if (!LOGGABLE_COM_IS_OK(hr)) {
        return false;
    }
    AM_MEDIA_TYPE* pmt = NULL;
    VIDEO_STREAM_CONFIG_CAPS caps;
    // get the windows capability from the capture device
    hr = streamConfig->GetStreamCaps(windowsCapability.directShowCapabilityIndex,
                                     &pmt, reinterpret_cast<BYTE*>(&caps));
    if (!LOGGABLE_COM_IS_OK(hr)) {
        return false;
    }
    if (pmt->formattype == FORMAT_VideoInfo2) {
        VIDEOINFOHEADER2* h = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->pbFormat);
        if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
            h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
        }
    }
    else {
        VIDEOINFOHEADER* h = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
        if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
            h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
        }
    }
    // Set the sink filter to request this capability
    hr = _sinkFilter->setRequestedCapability(capability);
    if (!LOGGABLE_COM_IS_OK(hr)) {
        webrtc::videocapturemodule::FreeMediaType(pmt);
        return false;
    }
    bool connected = false;
    const bool isDVCamera = pmt->subtype == MEDIASUBTYPE_dvsl ||
                            pmt->subtype == MEDIASUBTYPE_dvsd ||
                            pmt->subtype == MEDIASUBTYPE_dvhd;
    if (isDVCamera) {
        LOCK_WRITE_SAFE_OBJ(_dvCameraConfig);
        if (!_dvCameraConfig.constRef()) {
            connected = connectDVCamera(_graphBuilder,
                _inputSendPin, _outputCapturePin,
                _dvCameraConfig.ref(), logger());
        }
        else {
            connected = _dvCameraConfig.constRef()->connect();
        }
    }
    else {
        hr = _graphBuilder->ConnectDirect(_outputCapturePin, _inputSendPin, NULL);
        connected = LOGGABLE_COM_IS_OK(hr);
    }
    if (connected) {
        _requestedCapability(capability);
    }
    webrtc::videocapturemodule::FreeMediaType(pmt);
    return connected;
}

void WinCameraCapturer::disconnect()
{
    LOGGABLE_COM_ERROR(_graphBuilder->Disconnect(_outputCapturePin));
    if (_inputSendPin) {
        LOGGABLE_COM_ERROR(_graphBuilder->Disconnect(_inputSendPin));
    }
    LOCK_READ_SAFE_OBJ(_dvCameraConfig);
    if (const auto& dvCameraConfig = _dvCameraConfig.constRef()) {
        dvCameraConfig->disconnect();
    }
}

} // namespace LiveKitCpp
#endif
