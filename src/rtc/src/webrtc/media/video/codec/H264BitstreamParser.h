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
#pragma once // H264BitstreamParser.h
#include <api/rtc_error.h>
#include <api/scoped_refptr.h>
#include <api/video/encoded_image.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/h264/h264_common.h>
#include <memory>

namespace webrtc {
class EncodedImageBufferInterface;
}

namespace LiveKitCpp
{

class MemoryBlock;

class H264BitstreamParser
{
public:
    H264BitstreamParser();
    // quality
    void parseForSliceQp(webrtc::ArrayView<const uint8_t> bitstream);
    void parseForSliceQp(const uint8_t* bitstreamData, size_t bitstreamSize);
    void parseForSliceQp(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer);
    int lastSliceQp() const;
    void reset();
    // AnnexB processor
    static constexpr size_t annexBHeaderSize() { return sizeof(_annexBHeaderBytes); }
    static constexpr size_t naluParametersCount() { return 2UL; }
    static webrtc::RTCError addNaluForKeyFrame(MemoryBlock* targetBuffer,
                                               const uint8_t* naluBlock,
                                               const std::vector<webrtc::H264::NaluIndex>& indices);
    static webrtc::RTCError addNaluForKeyFrame(MemoryBlock* targetBuffer,
                                               const uint8_t* naluBlock,
                                               size_t naluBlockSize);
protected:
    static constexpr size_t avccHeaderByteSize() { return sizeof(uint32_t); }
    static void addData(MemoryBlock* buffer, const uint8_t* data, size_t size);
private:
    static const char _annexBHeaderBytes[4];
    std::unique_ptr<webrtc::H264BitstreamParser> _impl;
};

} // namespace LiveKitCpp
