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
#include "H264BitstreamParser.h"
#include "H264Utils.h"
#include "MemoryBlock.h"
#include "VideoUtils.h"
#include "Utils.h"
#include <absl/base/internal/endian.h>
#include <api/video/encoded_image.h>

namespace LiveKitCpp
{

const char H264BitstreamParser::_annexBHeaderBytes[4] = {0, 0, 0, 1};

H264BitstreamParser::H264BitstreamParser()
{
    reset();
}

void H264BitstreamParser::parseForSliceQp(webrtc::ArrayView<const uint8_t> bitstream)
{
    _impl->ParseBitstream(std::move(bitstream));
}

void H264BitstreamParser::parseForSliceQp(const uint8_t* bitstreamData, size_t bitstreamSize)
{
    parseForSliceQp(webrtc::MakeArrayView(bitstreamData, bitstreamSize));
}

void H264BitstreamParser::parseForSliceQp(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer)
{
    if (buffer) {
        parseForSliceQp(buffer->data(), buffer->size());
    }
}

int H264BitstreamParser::lastSliceQp() const
{
    return bound(H264Utils::lowQpThreshold(), _impl->GetLastSliceQp().value_or(-1), H264Utils::highQpThreshold());
}

void H264BitstreamParser::reset()
{
    _impl = std::make_unique<webrtc::H264BitstreamParser>();
}

webrtc::RTCError H264BitstreamParser::addNaluForKeyFrame(MemoryBlock* targetBuffer,
                                                         const uint8_t* naluBlock,
                                                         const std::vector<webrtc::H264::NaluIndex>& indices)
{
    if (targetBuffer && naluBlock) {
        if (naluParametersCount() == indices.size()) {
            for (const auto& index : indices) {
                addData(targetBuffer, naluBlock + index.payload_start_offset, index.payload_size);
            }
            return {};
        }
        return webrtc::RTCError(webrtc::RTCErrorType::INVALID_RANGE);
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_PARAMETER);
}

webrtc::RTCError H264BitstreamParser::addNaluForKeyFrame(MemoryBlock* targetBuffer,
                                                         const uint8_t* naluBlock,
                                                         size_t naluBlockSize)
{
    if (targetBuffer && naluBlock && naluBlockSize) {
        const webrtc::ArrayView<const uint8_t> nalu(naluBlock, naluBlockSize);
        const auto indices = webrtc::H264::FindNaluIndices(nalu);
        return addNaluForKeyFrame(targetBuffer, naluBlock, indices);
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_PARAMETER);
}

void H264BitstreamParser::addData(MemoryBlock* buffer, const uint8_t* data, size_t size)
{
    if (buffer) {
        buffer->appendData(_annexBHeaderBytes, annexBHeaderSize());
        buffer->appendData(data, size);
    }
}

} // namespace LiveKitCpp
