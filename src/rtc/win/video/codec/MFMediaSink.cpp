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
#include "MFMediaSink.h"

namespace LiveKitCpp 
{

HRESULT MFMediaSink::registerEncodingCallback(MFTEncodingCallback* callback)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_SINK_NO_STREAMS; // or E_INVALIDARG
        }
        else {
            _outputStream->registerEncodingCallback(callback);
        }
    }
    return hr;
}

HRESULT MFMediaSink::GetCharacteristics(DWORD* characteristics)
{
    if (!characteristics) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        // Rateless sink.
        *characteristics = MEDIASINK_RATELESS;
    }
    return hr;
}

HRESULT MFMediaSink::AddStreamSink(DWORD streamSinkIdentifier, 
                                   IMFMediaType* mediaType, 
                                   IMFStreamSink** outStreamSink)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (_outputStream) {
            hr = MF_E_STREAMSINK_EXISTS;
        }
        else {
            hr = Microsoft::WRL::MakeAndInitialize<LiveKitCpp::MFTStreamSink>(
                    &_outputStream, streamSinkIdentifier, this);
        }
        if (SUCCEEDED(hr)) {
            if (mediaType) {
                hr = _outputStream->SetCurrentMediaType(mediaType);
            }
        }
        // setup of output var doesn't presented in original code, 
        // remove it if any issues
        if (SUCCEEDED(hr) && outStreamSink) {
            *outStreamSink = _outputStream.Get();
            if (*outStreamSink) {
                (*outStreamSink)->AddRef();
            }
        }
    }
    return hr;
}

HRESULT MFMediaSink::RemoveStreamSink(DWORD streamSinkIdentifier)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_STREAMSINK_REMOVED; // or E_INVALIDARG
        }
        else {
            DWORD currentSinkId;
            hr = _outputStream->GetIdentifier(&currentSinkId);
            if (FAILED(hr) || currentSinkId != streamSinkIdentifier) {
                hr = E_INVALIDARG;
            }
            else {
                _outputStream->shutdown();
                _outputStream.Reset();
            }
        }
    }
    return hr;
}

HRESULT MFMediaSink::GetStreamSinkCount(DWORD* streamSinkCount)
{
    if (!streamSinkCount) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        *streamSinkCount = _outputStream ? 1U : 0U;
    }
    return hr;
}

HRESULT MFMediaSink::GetStreamSinkByIndex(DWORD index, IMFStreamSink** outStreamSink)
{
    if (!outStreamSink) {
        return E_INVALIDARG;
    }
    if (index > 0U) {
        return MF_E_INVALIDINDEX;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_INVALIDSTREAMNUMBER; // or E_INVALIDARG
        }
        else {
            *outStreamSink = _outputStream.Get();
            (*outStreamSink)->AddRef();
        }
    }
    return hr;
}

HRESULT MFMediaSink::GetStreamSinkById(DWORD identifier, IMFStreamSink** outStreamSink)
{
    if (!outStreamSink) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_INVALIDSTREAMNUMBER; // or E_INVALIDARG
        }
        else {
            DWORD currentSinkId;
            hr = _outputStream->GetIdentifier(&currentSinkId);
            if (FAILED(hr) || currentSinkId != identifier) {
                hr = E_INVALIDARG;
            }
            else {
                *outStreamSink = _outputStream.Get();
                (*outStreamSink)->AddRef();
            }
        }
    }
    return hr;
}

HRESULT MFMediaSink::SetPresentationClock(IMFPresentationClock* presentationClock)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (_clock) {
            hr = _clock->RemoveClockStateSink(this);
        }
        if (SUCCEEDED(hr)) {
            if (presentationClock) {
                hr = presentationClock->AddClockStateSink(this);
            }
        }
        if (SUCCEEDED(hr)) {
            _clock = presentationClock;
        }
    }
    return hr;
}

HRESULT MFMediaSink::GetPresentationClock(IMFPresentationClock** outPresentationClock)
{
    if (!outPresentationClock) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_clock) {
            hr = MF_E_NO_CLOCK;
        }
        else {
            *outPresentationClock = _clock.Get();
            (*outPresentationClock)->AddRef();
        }
    }
    return hr;
}

HRESULT MFMediaSink::Shutdown()
{
    const webrtc::MutexLock locker(&_lock);
    if (SUCCEEDED(checkShutdown())) {
        if (_outputStream) {
            _outputStream->shutdown();
            _outputStream.Reset();
        }
        _clock.Reset();
        _shutdown = true;
    }
    return S_OK;
}

HRESULT MFMediaSink::OnClockStart(MFTIME /*hnsSystemTime*/,
                                  LONGLONG clockStartOffset)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_SINK_NO_STREAMS; // or E_INVALIDARG
        }
        else {
            hr = _outputStream->start(clockStartOffset);
        }
    }
    return hr;
}

HRESULT MFMediaSink::OnClockStop(MFTIME /*hnsSystemTime*/)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_outputStream) {
            hr = MF_E_SINK_NO_STREAMS; // or E_INVALIDARG
        }
        else {
            hr = _outputStream->stop();
        }
    }
    return S_OK;
}

} // namespace LiveKitCpp