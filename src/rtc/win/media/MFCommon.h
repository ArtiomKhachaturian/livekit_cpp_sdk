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
#pragma once // MFCommon.h
#include <api/rtc_error.h>
#include <atlbase.h> //CComPtr support
#include <mfobjects.h>
#include <mftransform.h>
#include <string>

namespace webrtc {
enum class VideoType;
}

namespace LiveKitCpp 
{

class MFTransformConfigurator;

// transform
webrtc::RTCErrorOr<CComPtr<IMFTransform>> createTransform(const GUID& compressedType,
                                                          bool video,
                                                          bool encoder,
                                                          UINT32 desiredFlags,
                                                          const GUID& uncompressedType = GUID_NULL, // maybe GUID_NULL
                                                          DWORD* inputStreamID = NULL,
                                                          DWORD* outputStreamID = NULL,
                                                          UINT32* actualFlags = nullptr,
                                                          std::string* friendlyName = nullptr,
                                                          MFTransformConfigurator* configurator = nullptr);
// 1st - input ID, 2nd - output
webrtc::RTCErrorOr<std::pair<DWORD, DWORD>> transformStreamIDs(const CComPtr<IMFTransform>& transform);
webrtc::RTCErrorOr<std::string> transformFriendlyName(IMFAttributes* attributes);
// media type
webrtc::RTCErrorOr<CComPtr<IMFMediaType>> createMediaType(bool video, const GUID& subType);
webrtc::RTCError setAllSamplesIndependent(const CComPtr<IMFMediaType>& mediaType, bool set);
webrtc::RTCError setFrameSize(const CComPtr<IMFMediaType>& mediaType, UINT32 width, UINT32 height);
webrtc::RTCError setFramerate(const CComPtr<IMFMediaType>& mediaType, UINT32 num, UINT32 denum);
webrtc::RTCError setFramerate(const CComPtr<IMFMediaType>& mediaType, UINT32 frameRate);
webrtc::RTCError setPixelAspectRatio(const CComPtr<IMFMediaType>& mediaType, UINT32 num, UINT32 denum);
webrtc::RTCError setPixelAspectRatio1x1(const CComPtr<IMFMediaType>& mediaType);
// 1st - width, 2nd - height
webrtc::RTCErrorOr<std::pair<UINT32, UINT32>> frameSize(const CComPtr<IMFMediaType>& mediaType);
webrtc::RTCErrorOr<UINT32> framerate(const CComPtr<IMFMediaType>& mediaType);
// sample
webrtc::RTCErrorOr<CComPtr<IMFSample>> createSample(const CComPtr<IMFMediaBuffer>& attachedBuffer = CComPtr<IMFMediaBuffer>());
webrtc::RTCErrorOr<CComPtr<IMFSample>> createSampleWitMemoryBuffer(DWORD maxLength, DWORD aligment = 0UL);
// flags tester
bool acceptFlags(DWORD desired, DWORD actual);

} // namespace LiveKitCpp