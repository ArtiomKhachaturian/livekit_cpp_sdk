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
#pragma once // MacH264BitstreamParser.h
#include "H264BitstreamParser.h"
#include <CoreMedia/CoreMedia.h>
#include <CoreMedia/CMSampleBuffer.h> // for CMSampleBufferRef

namespace LiveKitCpp
{

using CMVideoFormat = webrtc::RTCErrorOr<CMVideoFormatDescriptionRef>;

class MacH264BitstreamParser : public H264BitstreamParser
{
public:
    static webrtc::RTCError addNaluForKeyFrame(MemoryBlock* targetBuffer, CMSampleBufferRef sampleBuffer);
    static webrtc::RTCError storeAnnexBFrame(MemoryBlock* targetBuffer, CMSampleBufferRef sampleBuffer);
    static webrtc::RTCError storeAnnexBFrame(MemoryBlock* targetBuffer, CMBlockBufferRef sampleDataContiguousBuffer);
    static webrtc::RTCError storeAnnexBFrame(MemoryBlock* targetBuffer, const uint8_t* sampleData, size_t sampleDataSize);
    // Create a CMFormatDescription using the provided |pps| and |sps|.
    static CMVideoFormat createVideoFormatH264(const std::vector<uint8_t>& sps,
                                               const std::vector<uint8_t>& pps,
                                               const std::vector<uint8_t>& spsext = {});
    static CMVideoFormat createVideoFormatH264(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& annexBBuffer);
};

} // namespace LiveKitCpp
