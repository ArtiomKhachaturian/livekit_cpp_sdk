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
#include "CompletionStatusOr.h"
#include <api/video/video_codec_type.h>
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
CompletionStatusOrComPtr<IMFTransform> createTransform(const GUID& compressedType,
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
CompletionStatusOr<std::pair<DWORD, DWORD>> transformStreamIDs(IMFTransform* transform);
CompletionStatusOr<std::string> transformFriendlyName(IMFAttributes* attributes);
CompletionStatusOr<std::string> transformFriendlyName(IMFTransform* transform);
// media type
CompletionStatusOrComPtr<IMFMediaType> createMediaType(bool video, const GUID& subType);
CompletionStatus setAllSamplesIndependent(IMFMediaType* mediaType, bool set);
CompletionStatus setFrameSize(IMFMediaType* mediaType, UINT32 width, UINT32 height);
CompletionStatus setFramerate(IMFMediaType* mediaType, UINT32 num, UINT32 denum);
CompletionStatus setFramerate(IMFMediaType* mediaType, UINT32 frameRate);
CompletionStatus setPixelAspectRatio(IMFMediaType* mediaType, UINT32 num, UINT32 denum);
CompletionStatus setPixelAspectRatio1x1(IMFMediaType* mediaType);
CompletionStatus setInterlaceMode(IMFMediaType* mediaType, MFVideoInterlaceMode mode);
// 1st - width, 2nd - height
CompletionStatusOr<std::pair<UINT32, UINT32>> frameSize(IMFMediaType* mediaType);
CompletionStatusOr<UINT32> framerate(IMFMediaType* mediaType);
// sample
CompletionStatusOrComPtr<IMFSample> createSample(IMFMediaBuffer* attachedBuffer = {});
CompletionStatusOrComPtr<IMFSample> createSampleWitMemoryBuffer(DWORD maxLength, DWORD aligment = 0UL);
// flags tester
bool acceptFlags(DWORD desired, DWORD actual);
const GUID& predefinedCodecType(bool encoder, const GUID& compressedType);
const GUID& compressedType(webrtc::VideoCodecType type);

} // namespace LiveKitCpp