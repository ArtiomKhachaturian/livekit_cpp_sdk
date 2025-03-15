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
#include "CaptureInputPin.h"
#include "CaptureSinkFilter.h"
#include "CameraManager.h"
#include "ScopedCoMem.h"
#include "CameraErrorHandling.h"
#include "Utils.h"
#include <dvdmedia.h>
#include <modules/video_capture/windows/help_functions_ds.h>

namespace {

inline bool isMediaTypeFullySpecified(const AM_MEDIA_TYPE& type)
{
    return type.majortype != GUID_NULL && type.formattype != GUID_NULL;
}

void getSampleProperties(IMediaSample* sample, AM_SAMPLE2_PROPERTIES* props);

VIDEOINFOHEADER* allocMediaTypeFormatBuffer(AM_MEDIA_TYPE* mediaType);

void setMediaInfoFromVideoType(webrtc::VideoType videoType,
                               BITMAPINFOHEADER* bitmapHeader,
                               AM_MEDIA_TYPE* mediaType);

bool isMediaTypePartialMatch(const AM_MEDIA_TYPE& a, const AM_MEDIA_TYPE& b);
// Returns true if the media type is supported, false otherwise.
// For supported types, the `capability` will be populated accordingly.
bool translateMediaTypeToVideoCaptureCapability(
    const AM_MEDIA_TYPE* mediaType,
    webrtc::VideoCaptureCapability& capability);

} // namespace

namespace LiveKitCpp 
{

class CaptureInputPin::MediaTypesEnum : public IUnknownImpl<IEnumMediaTypes>
{
public:
    MediaTypesEnum(const webrtc::VideoCaptureCapability& capability);
    // impl. of IEnumMediaTypes
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes**) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Skip(ULONG) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Next(ULONG count, AM_MEDIA_TYPE** types, ULONG* fetched) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Reset() final;
protected:
    // impl. of IUnknownImpl
    void releaseThis() final { delete this; }
private:
    const webrtc::VideoCaptureCapability _capability;
    // default preferences, sorted by cost-to-convert-to-i420
    std::list<webrtc::VideoType> _formatPreferenceOrder;
    size_t _pos = 0UL;
};

CaptureInputPin::CaptureInputPin(CaptureSinkFilter* filter,
                                 const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
    , _info{filter, PINDIR_INPUT}
{
    ZeroMemory(&_mediaType.ref(), sizeof(AM_MEDIA_TYPE));
}

CaptureInputPin::~CaptureInputPin()
{
    resetMediaType();
}

HRESULT CaptureInputPin::SetRequestedCapability(const webrtc::VideoCaptureCapability& capability)
{
    HRESULT hr = VFW_E_NOT_STOPPED;
    if (filter()->isStopped()) {
        {
            LOCK_WRITE_SAFE_OBJ(_requestedCapability);
            _requestedCapability.ref() = capability;
        }
        LOCK_WRITE_SAFE_OBJ(_resultingCapability);
        _resultingCapability = webrtc::VideoCaptureCapability();
        hr = S_OK;
    }
    return hr;
}

void CaptureInputPin::notifyThatFilterActivated(bool activated)
{
    if (activated) {
        _runtimeError = false;
        _flushing = false;
        _captureThreadId = 0UL;
    } else {
        _flushing = true;
        LOCK_READ_SAFE_OBJ(_allocator);
        if (const auto& allocator = _allocator.constRef()) {
            LOGGABLE_COM_ERROR(allocator->Decommit());
        }
    }
}

HRESULT CaptureInputPin::Connect(IPin* receivePin, const AM_MEDIA_TYPE* mediaType)
{
    HRESULT hr = E_POINTER;
    if (receivePin && mediaType) {
        if (filter()->isStopped()) {
            LOCK_WRITE_SAFE_OBJ(_receivePin);
            if (!_receivePin.constRef()) {
                if (isMediaTypeFullySpecified(*mediaType)) {
                    hr = attemptConnection(receivePin, mediaType);
                    if (LOGGABLE_COM_IS_OK(hr)) {
                        _receivePin.ref() = receivePin;
                        resetMediaType(mediaType);
                    }
                } else {
                    AM_MEDIA_TYPE* acceptedMediaType = nullptr;
                    const auto types = determineCandidateFormats(receivePin, mediaType);
                    for (const auto type : types) {
                        if (acceptedMediaType == nullptr && LOGGABLE_COM_IS_OK(attemptConnection(receivePin, type))) {
                            acceptedMediaType = type;
                        }
                        if (type != acceptedMediaType) {
                            webrtc::videocapturemodule::FreeMediaType(type);
                        }
                    }
                    if (acceptedMediaType == nullptr) {
                        hr = VFW_E_NO_ACCEPTABLE_TYPES;
                    } else {
                        hr = S_OK;
                        _receivePin.ref() = receivePin;
                        resetMediaType(acceptedMediaType);
                        webrtc::videocapturemodule::FreeMediaType(acceptedMediaType);
                    }
                }
            } else {
                hr = VFW_E_ALREADY_CONNECTED;
            }
        } else {
            hr = VFW_E_NOT_STOPPED;
        }
    }
    return hr;
}

HRESULT CaptureInputPin::ReceiveConnection(IPin* connector, const AM_MEDIA_TYPE* mediaType)
{
    HRESULT hr = E_POINTER;
    if (connector && mediaType) {
        if (filter()->isStopped()) {
            LOCK_WRITE_SAFE_OBJ(_receivePin);
            if (_receivePin.constRef()) {
                hr = VFW_E_ALREADY_CONNECTED;
            } else {
                hr = checkDirection(connector);
                if (LOGGABLE_COM_IS_OK(hr)) {
                    webrtc::VideoCaptureCapability capability;
                    if (translateMediaTypeToVideoCaptureCapability(mediaType, capability)) {
                        LOCK_WRITE_SAFE_OBJ(_resultingCapability);
                        _resultingCapability = std::move(capability);
                        _receivePin = connector;
                        resetMediaType(mediaType);
                    } else {
                        hr = VFW_E_TYPE_NOT_ACCEPTED;
                    }
                }
            }
        } else {
            hr = VFW_E_NOT_STOPPED;
        }
    }
    return hr;
}

HRESULT CaptureInputPin::Disconnect()
{
    if (filter()->isStopped()) {
        LOCK_WRITE_SAFE_OBJ(_receivePin);
        if (auto& receivePin = _receivePin.ref()) {
            receivePin.Release();
            clearAllocator(true);
            return S_OK;
        }
        return S_FALSE;
    }
    return VFW_E_NOT_STOPPED;
}

HRESULT CaptureInputPin::ConnectedTo(IPin** pin)
{
    if (pin) {
        LOCK_READ_SAFE_OBJ(_receivePin);
        if (const auto& receivePin = _receivePin.constRef()) {
            *pin = receivePin;
            (*pin)->AddRef();
            return S_OK;
        }
        return VFW_E_NOT_CONNECTED;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::ConnectionMediaType(AM_MEDIA_TYPE* mediaType)
{
    if (mediaType) {
        LOCK_READ_SAFE_OBJ(_receivePin);
        if (_receivePin.constRef()) {
            resetMediaType(mediaType); // CopyMediaType(media_type, &media_type_);???
            return S_OK;
        }
        return VFW_E_NOT_CONNECTED;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::QueryPinInfo(PIN_INFO* info)
{
    if (info) {
        *info = _info;
        if (_info.pFilter) {
            _info.pFilter->AddRef();
        }
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::QueryDirection(PIN_DIRECTION* pinDir)
{
    if (pinDir) {
        *pinDir = _info.dir;
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::QueryId(LPWSTR* id)
{
    if (id) {
        const size_t len = ::lstrlenW(_info.achName);
        const auto buffer = reinterpret_cast<LPWSTR>(::CoTaskMemAlloc((len + 1UL) * sizeof(wchar_t)));
        if (buffer) {
            ::lstrcpyW(buffer, _info.achName);
            *id = buffer;
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::QueryAccept(const AM_MEDIA_TYPE* mediaType)
{
    if (mediaType) {
        if (filter()->isStopped()) {
            LOCK_READ_SAFE_OBJ(_resultingCapability);
            webrtc::VideoCaptureCapability capability(_resultingCapability.constRef());
            if (translateMediaTypeToVideoCaptureCapability(mediaType, capability)) {
                return S_OK;
            }
            return S_FALSE;
        }
        return VFW_E_NOT_STOPPED;
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::EnumMediaTypes(IEnumMediaTypes** outputEnumerator)
{
    if (outputEnumerator) {
        *outputEnumerator = createMediaTypesEnumerator();
        if (*outputEnumerator) {
            (*outputEnumerator)->AddRef(); // ???
            return S_OK;
        }
    }
    return E_POINTER;
}

HRESULT CaptureInputPin::BeginFlush()
{
    _flushing = true;
    return S_OK;
}

HRESULT CaptureInputPin::EndFlush()
{
    _flushing = false;
    _runtimeError = false;
    return S_OK;
}

HRESULT CaptureInputPin::NotifyAllocator(IMemAllocator* allocator, BOOL)
{
    LOCK_WRITE_SAFE_OBJ(_allocator);
    _allocator = allocator;
    return S_OK;
}

HRESULT CaptureInputPin::GetAllocator(IMemAllocator** allocator)
{
    HRESULT hr = E_POINTER;
    if (allocator) {
        LOCK_WRITE_SAFE_OBJ(_allocator);
        if (!_allocator.constRef()) {
            hr = _allocator.ref().CoCreateInstance(CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER);
        } else {
            hr = S_OK;
        }
        if (LOGGABLE_COM_IS_OK(hr)) {
            *allocator = _allocator.constRef();
            (*allocator)->AddRef();
        }
    }
    return hr;
}

HRESULT CaptureInputPin::Receive(IMediaSample* sample)
{
    HRESULT hr = E_POINTER;
    if (sample) {
        if (hasFlushing()) {
            hr = S_FALSE;
        } else if (hasRuntimeError()) {
            hr = VFW_E_RUNTIME_ERROR;
        } else {
            hr = S_OK;
            // make sure we set the thread name only once
            static thread_local const DWORD captureThreadId = GetCurrentThreadId();
            DWORD noThreadId = 0UL;
            if (_captureThreadId.compare_exchange_strong(noThreadId, captureThreadId)) {
                // TODO: fix it later
                //setCurrentThreadName("webrtc_video_capture");
            }
            AM_SAMPLE2_PROPERTIES sampleProps = {};
            getSampleProperties(sample, &sampleProps);
            // has the format changed in this sample?
            if (sampleProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) {
                LOCK_WRITE_SAFE_OBJ(_resultingCapability);
                if (!translateMediaTypeToVideoCaptureCapability(sampleProps.pMediaType, _resultingCapability.ref())) {
                    _runtimeError = true;
                    EndOfStream();
                    filter()->notifyEvent(EC_ERRORABORT, VFW_E_TYPE_NOT_ACCEPTED, 0);
                    hr = VFW_E_INVALIDMEDIATYPE;
                } else {
                    //resetMediaType(sampleProps.pMediaType);
                }
            }
            if (S_OK == hr) {
                LOCK_READ_SAFE_OBJ(_resultingCapability);
                filter()->deliverFrame(sampleProps.pbBuffer, sampleProps.lActual,
                                       sampleProps.cbBuffer, sample,
                                       _resultingCapability.constRef());
            }
        }
    }
    return hr;
}

HRESULT CaptureInputPin::ReceiveMultiple(IMediaSample** samples, long count, long* processed)
{
    HRESULT hr = E_POINTER;
    if (samples) {
        if (count > 0UL) {
            if (processed) {
                *processed = 0UL;
            }
            if (hasFlushing()) {
                hr = S_FALSE;
            } else if (hasRuntimeError()) {
                hr = VFW_E_RUNTIME_ERROR;
            } else {
                size_t index = 0UL;
                while (count-- > 0UL) {
                    hr = Receive(samples[index++]);
                    if (S_OK != hr) {
                        break;
                    }
                    if (processed) {
                        ++(*processed);
                    }
                }
            }
        } else {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

std::string_view CaptureInputPin::logCategory() const
{
    return CameraManager::logCategory();
}

CaptureSinkFilter* CaptureInputPin::filter() const
{
    return static_cast<CaptureSinkFilter*>(_info.pFilter);
}

void CaptureInputPin::resetMediaType(const AM_MEDIA_TYPE* newMediaType)
{
    LOCK_WRITE_SAFE_OBJ(_mediaType);
    webrtc::videocapturemodule::ResetMediaType(&_mediaType.ref());
    if (newMediaType != nullptr) {
        webrtc::videocapturemodule::CopyMediaType(&_mediaType.ref(), newMediaType);
    }
}

HRESULT CaptureInputPin::checkDirection(IPin* pin) const
{
    HRESULT hr = E_POINTER;
    if (pin) {
        PIN_DIRECTION pd;
        hr = pin->QueryDirection(&pd);
        if (LOGGABLE_COM_IS_OK(hr)) {
            // fairly basic check, make sure we don't pair input with input etc.
            hr = pd == _info.dir ? VFW_E_INVALID_DIRECTION : S_OK;
        }
    }
    return hr;
}

void CaptureInputPin::clearAllocator(bool decommit)
{
    LOCK_WRITE_SAFE_OBJ(_allocator);
    if (auto& allocator = _allocator.ref()) {
        if (decommit) {
            LOGGABLE_COM_ERROR(allocator->Decommit());
        }
        allocator.Release();
    }
}

IEnumMediaTypes* CaptureInputPin::createMediaTypesEnumerator() const
{
    LOCK_READ_SAFE_OBJ(_requestedCapability);
    return new MediaTypesEnum(_requestedCapability.constRef());
}

HRESULT CaptureInputPin::attemptConnection(IPin* receivePin, const AM_MEDIA_TYPE* mediaType)
{
    HRESULT hr = E_POINTER;
    if (receivePin && mediaType) {
        if (filter()->isStopped()) {
            hr = checkDirection(receivePin);
            if (LOGGABLE_COM_IS_OK(hr)) {
                webrtc::VideoCaptureCapability capability;
                if (translateMediaTypeToVideoCaptureCapability(mediaType, capability)) {
                    hr = receivePin->ReceiveConnection(this, mediaType);
                    if (LOGGABLE_COM_IS_OK(hr)) {
                        LOCK_WRITE_SAFE_OBJ(_resultingCapability);
                        _resultingCapability = std::move(capability);
                    }
                } else {
                    clearAllocator(true); // ???
                    hr = VFW_E_TYPE_NOT_ACCEPTED;
                }
            }

        } else {
            hr = VFW_E_NOT_STOPPED;
        }
    }
    return hr;
}

std::vector<AM_MEDIA_TYPE*> CaptureInputPin::determineCandidateFormats(IPin* receivePin,
                                                                       const AM_MEDIA_TYPE* mediaType) const
{
    if (receivePin && mediaType) {
        std::vector<AM_MEDIA_TYPE*> ret;
        for (int i = 0; i < 2; i++) {
            CComPtr<IEnumMediaTypes> types;
            if (i == 0) {
                // first time around, try types from receive_pin
                LOGGABLE_COM_ERROR(receivePin->EnumMediaTypes(&types));
            } else {
                types = createMediaTypesEnumerator();
            }
            if (types) {
                while (true) {
                    ULONG fetched = 0UL;
                    AM_MEDIA_TYPE* thisType = nullptr;
                    if (S_OK != types->Next(1, &thisType, &fetched)) {
                        break;
                    }
                    if (isMediaTypePartialMatch(*thisType, *mediaType)) {
                        ret.push_back(thisType);
                    } else {
                        webrtc::videocapturemodule::FreeMediaType(thisType);
                    }
                }
            }
        }
        return ret;
    }
    return {};
}

CaptureInputPin::MediaTypesEnum::MediaTypesEnum(const webrtc::VideoCaptureCapability& capability)
    : _capability(capability)
    , _formatPreferenceOrder({webrtc::VideoType::kI420, webrtc::VideoType::kYUY2,
                              webrtc::VideoType::kRGB24, webrtc::VideoType::kUYVY, webrtc::VideoType::kMJPEG})
{
    // use the preferred video type, if supported
    auto it = std::find(_formatPreferenceOrder.begin(), _formatPreferenceOrder.end(), _capability.videoType);
    if (it != _formatPreferenceOrder.end()) {
        // move it to the front of the list, if it isn't already there
        if (it != _formatPreferenceOrder.begin()) {
            _formatPreferenceOrder.splice(_formatPreferenceOrder.begin(), _formatPreferenceOrder, it, std::next(it));
        }
    }
}

HRESULT CaptureInputPin::MediaTypesEnum::Next(ULONG count, AM_MEDIA_TYPE** types, ULONG* fetched)
{
    HRESULT hr = E_POINTER;
    if (types) {
        if (count > 0UL) {
            if (fetched) { // fetched may be NULL
                *fetched = 0UL;
            }
            for (ULONG i = 0; i < count && _pos < _formatPreferenceOrder.size(); ++i) {
                auto mediaType = ScopedCoMem<AM_MEDIA_TYPE>::alloc();
                if (mediaType) {
                    const auto vih = allocMediaTypeFormatBuffer(mediaType);
                    if (vih) {
                        if (0 != _capability.maxFPS) {
                            vih->AvgTimePerFrame = 10000000 / _capability.maxFPS;
                        }
                        mediaType->majortype = MEDIATYPE_Video;
                        mediaType->formattype = FORMAT_VideoInfo;
                        mediaType->bTemporalCompression = FALSE;
                        // set format information.
                        auto formatIt = std::next(_formatPreferenceOrder.begin(), _pos++);
                        setMediaInfoFromVideoType(*formatIt, &vih->bmiHeader, mediaType);
                        vih->bmiHeader.biWidth = _capability.width;
                        vih->bmiHeader.biHeight = _capability.height;
                        vih->bmiHeader.biSizeImage = ((vih->bmiHeader.biBitCount / 4) *
                                                      _capability.height * _capability.width) /
                                                     2;

                        assert(vih->bmiHeader.biSizeImage > 0UL);
                        mediaType->lSampleSize = vih->bmiHeader.biSizeImage;
                        mediaType->bFixedSizeSamples = true;
                        types[i] = mediaType.take();
                        if (fetched) {
                            ++(*fetched);
                        }
                    } else {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                } else {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            if (E_OUTOFMEMORY != hr) {
                hr = _pos == _formatPreferenceOrder.size() ? S_FALSE : S_OK;
            }
        } else {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

HRESULT CaptureInputPin::MediaTypesEnum::Reset()
{
    _pos = 0UL;
    return S_OK;
}

} // namespace LiveKitCpp

namespace {

void getSampleProperties(IMediaSample* sample, AM_SAMPLE2_PROPERTIES* props)
{
    if (sample && props) {
        CComPtr<IMediaSample2> sample2;
        HRESULT hr = sample->QueryInterface(&sample2);
        if (SUCCEEDED(hr)) {
            sample2->GetProperties(sizeof(*props), reinterpret_cast<BYTE*>(props));
        } else {
            //  get the properties the hard way
            props->cbData = sizeof(AM_SAMPLE2_PROPERTIES);
            props->dwTypeSpecificFlags = 0UL;
            props->dwStreamId = AM_STREAM_MEDIA;
            props->dwSampleFlags = 0UL;

            if (sample->IsDiscontinuity() == S_OK) {
                props->dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
            }

            if (sample->IsPreroll() == S_OK) {
                props->dwSampleFlags |= AM_SAMPLE_PREROLL;
            }

            if (sample->IsSyncPoint() == S_OK) {
                props->dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
            }

            if (SUCCEEDED(sample->GetTime(&props->tStart, &props->tStop))) {
                props->dwSampleFlags |= AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID;
            }

            if (sample->GetMediaType(&props->pMediaType) == S_OK) {
                props->dwSampleFlags |= AM_SAMPLE_TYPECHANGED;
            }

            sample->GetPointer(&props->pbBuffer);
            props->lActual = sample->GetActualDataLength();
            props->cbBuffer = sample->GetSize();
        }
    }
}

VIDEOINFOHEADER* allocMediaTypeFormatBuffer(AM_MEDIA_TYPE* mediaType)
{
    if (mediaType) {
        static const size_t length = sizeof(VIDEOINFOHEADER);
        auto buffer = static_cast<BYTE*>(::CoTaskMemAlloc(length));
        if (buffer) {
            ZeroMemory(buffer, length);
            if (mediaType->pbFormat) {
                ::CoTaskMemFree(mediaType->pbFormat);
            }
            mediaType->pbFormat = buffer;
            mediaType->cbFormat = length;
            const auto vih = reinterpret_cast<VIDEOINFOHEADER*>(buffer);
            vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            vih->bmiHeader.biPlanes = 1;
            vih->bmiHeader.biClrImportant = 0;
            vih->bmiHeader.biClrUsed = 0;
            ::SetRectEmpty(&vih->rcSource); // we want the whole image area rendered
            ::SetRectEmpty(&vih->rcTarget); // no particular destination rectangle
            return vih;
        }
    }
    return nullptr;
}

void setMediaInfoFromVideoType(webrtc::VideoType videoType,
                               BITMAPINFOHEADER* bitmapHeader,
                               AM_MEDIA_TYPE* mediaType)
{
    if (bitmapHeader && mediaType) {
        switch (videoType) {
        case webrtc::VideoType::kI420:
            bitmapHeader->biCompression = MAKEFOURCC('I', '4', '2', '0');
            bitmapHeader->biBitCount = 12; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_I420;
            break;
        case webrtc::VideoType::kYUY2:
            bitmapHeader->biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
            bitmapHeader->biBitCount = 16; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_YUY2;
            break;
        case webrtc::VideoType::kRGB24:
            bitmapHeader->biCompression = BI_RGB;
            bitmapHeader->biBitCount = 24; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_RGB24;
            break;
        case webrtc::VideoType::kUYVY:
            bitmapHeader->biCompression = MAKEFOURCC('U', 'Y', 'V', 'Y');
            bitmapHeader->biBitCount = 16; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_UYVY;
            break;
        case webrtc::VideoType::kMJPEG:
            bitmapHeader->biCompression = MAKEFOURCC('M', 'J', 'P', 'G');
            bitmapHeader->biBitCount = 12; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_MJPG;
            break;
        case webrtc::VideoType::kNV12:
            bitmapHeader->biCompression = MAKEFOURCC('N', 'V', '1', '2');
            bitmapHeader->biBitCount = 12; // bit per pixel
            mediaType->subtype = MEDIASUBTYPE_NV12;
            break;
        default:
            break;
        }
    }
}

bool translateMediaTypeToVideoCaptureCapability(
    const AM_MEDIA_TYPE* mediaType,
    webrtc::VideoCaptureCapability& capability)
{
    if (mediaType && mediaType->pbFormat && MEDIATYPE_Video == mediaType->majortype) {
        const BITMAPINFOHEADER* bih = nullptr;
        if (mediaType->formattype == FORMAT_VideoInfo) {
            bih = &reinterpret_cast<VIDEOINFOHEADER*>(mediaType->pbFormat)->bmiHeader;
        } else if (mediaType->formattype != FORMAT_VideoInfo2) {
            bih = &reinterpret_cast<VIDEOINFOHEADER2*>(mediaType->pbFormat)->bmiHeader;
        } else {
            return false;
        }
        const GUID& sub_type = mediaType->subtype;
        if (sub_type == MEDIASUBTYPE_MJPG &&
            bih->biCompression == MAKEFOURCC('M', 'J', 'P', 'G')) {
            capability.videoType = webrtc::VideoType::kMJPEG;
        } else if (sub_type == MEDIASUBTYPE_I420 &&
                   bih->biCompression == MAKEFOURCC('I', '4', '2', '0')) {
            capability.videoType = webrtc::VideoType::kI420;
        } else if (sub_type == MEDIASUBTYPE_YUY2 &&
                   bih->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2')) {
            capability.videoType = webrtc::VideoType::kYUY2;
        } else if (sub_type == MEDIASUBTYPE_UYVY &&
                   bih->biCompression == MAKEFOURCC('U', 'Y', 'V', 'Y')) {
            capability.videoType = webrtc::VideoType::kUYVY;
        } else if (sub_type == MEDIASUBTYPE_HDYC) {
            capability.videoType = webrtc::VideoType::kUYVY;
        } else if (sub_type == MEDIASUBTYPE_RGB24 && bih->biCompression == BI_RGB) {
            capability.videoType = webrtc::VideoType::kRGB24;
        } else if (sub_type == MEDIASUBTYPE_NV12 &&
                   bih->biCompression == MAKEFOURCC('N', 'V', '1', '2')) {
            capability.videoType = webrtc::VideoType::kNV12;
        } else {
            return false;
        }

        // store the incoming width and height
        capability.width = bih->biWidth;

        // store the incoming height,
        // for RGB24 we assume the frame to be upside down
        if (sub_type == MEDIASUBTYPE_RGB24 && bih->biHeight > 0) {
            capability.height = -(bih->biHeight);
        } else {
            capability.height = abs(bih->biHeight);
        }

        return true;
    }
    return false;
}

bool isMediaTypePartialMatch(const AM_MEDIA_TYPE& a, const AM_MEDIA_TYPE& b)
{
    if (b.majortype != GUID_NULL && a.majortype != b.majortype) {
        return false;
    }
    if (b.subtype != GUID_NULL && a.subtype != b.subtype) {
        return false;
    }
    if (b.formattype != GUID_NULL) {
        // if the format block is specified then it must match exactly
        if (a.formattype != b.formattype) {
            return false;
        }
        if (a.cbFormat != b.cbFormat) {
            return false;
        }
        if (a.cbFormat != 0 && std::memcmp(a.pbFormat, b.pbFormat, a.cbFormat) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace
#endif