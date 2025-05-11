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
#include "MacH264BitstreamParser.h"
#ifdef USE_PLATFORM_ENCODERS
#include "CFAutoRelease.h"
#include "RtcUtils.h"
#include <components/video_codec/nalu_rewriter.h>
#include <cassert>

namespace LiveKitCpp
{

CompletionStatus MacH264BitstreamParser::addNaluForKeyFrame(MemoryBlock* targetBuffer,
                                                            CMSampleBufferRef sampleBuffer)
{
    if (targetBuffer && sampleBuffer) {
        if (const auto description = CMSampleBufferGetFormatDescription(sampleBuffer)) {
            // get parameter set information
            size_t paramSetCount = 0UL;
            int naluHeaderSize = 0;
            auto status = COMPLETION_STATUS(CMVideoFormatDescriptionGetH264ParameterSetAtIndex(description, 0,
                                                                                               nullptr, nullptr,
                                                                                               &paramSetCount,
                                                                                               &naluHeaderSize));
            if (status) {
                assert(naluHeaderSize == avccHeaderByteSize());
                assert(naluParametersCount() == paramSetCount);
                size_t paramSetSize = 0;
                const uint8_t* paramSet = nullptr;
                for (size_t i = 0UL; i < paramSetCount; ++i) {
                    status = COMPLETION_STATUS(CMVideoFormatDescriptionGetH264ParameterSetAtIndex(description, i,
                                                                                                  &paramSet,
                                                                                                  &paramSetSize,
                                                                                                  nullptr,
                                                                                                  nullptr));
                    if (!status) {
                        break;
                    }
                    // update buffer
                    addData(targetBuffer, paramSet, paramSetSize);
                }
            }
            return status;
        }
        return COMPLETION_STATUS(kCMSampleBufferError_InvalidMediaTypeForOperation);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MacH264BitstreamParser::storeAnnexBFrame(MemoryBlock* targetBuffer,
                                                          CMSampleBufferRef sampleBuffer)
{
    if (targetBuffer && sampleBuffer) {
        // get block buffer from the sample buffer.
        if (const auto blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer)) {
            CompletionStatus status;
            // make sure block buffer is contiguous
            CFAutoRelease<CMBlockBufferRef> contiguousBuffer;
            if (!CMBlockBufferIsRangeContiguous(blockBuffer, 0, 0)) {
                status = COMPLETION_STATUS(CMBlockBufferCreateContiguous(nullptr,
                                                                         blockBuffer,
                                                                         nullptr, nullptr,
                                                                         0UL, 0UL, 0U,
                                                                         contiguousBuffer.pointer()));
            } else {
                contiguousBuffer.set(blockBuffer, true);
            }
            if (status) {
                return storeAnnexBFrame(targetBuffer, contiguousBuffer);
            }
            return status;
        }
        return COMPLETION_STATUS(kCMBlockBufferEmptyBBufErr);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MacH264BitstreamParser::storeAnnexBFrame(MemoryBlock* targetBuffer,
                                                          CMBlockBufferRef sampleDataContiguousBuffer)
{
    if (targetBuffer && sampleDataContiguousBuffer) {
        const size_t blockBufferSize = CMBlockBufferGetDataLength(sampleDataContiguousBuffer);
        if (blockBufferSize) {
            // now copy the actual data
            char* data = nullptr;
            auto status = COMPLETION_STATUS(CMBlockBufferGetDataPointer(sampleDataContiguousBuffer, 0,
                                                                        nullptr, nullptr, &data));
            if (status) {
                return storeAnnexBFrame(targetBuffer, reinterpret_cast<const uint8_t*>(data), blockBufferSize);
            }
            return status;
        }
        return COMPLETION_STATUS(kCMBlockBufferBadLengthParameterErr);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MacH264BitstreamParser::storeAnnexBFrame(MemoryBlock* targetBuffer,
                                                          const uint8_t* sampleData,
                                                          size_t sampleDataSize)
{
    if (targetBuffer && sampleData) {
        if (sampleDataSize) {
            size_t bytesRemaining = sampleDataSize;
            while (bytesRemaining > 0UL) {
                // the size type here must match `_avccHeaderByteSize`, we expect 4 bytes,
                // read the length of the next packet of data and it must convert from big endian to host endian
                assert(bytesRemaining >= avccHeaderByteSize());
                const auto uint32DataPtr = reinterpret_cast<const uint32_t*>(sampleData);
                // converts a 32-bit integer from big-endian format to the host’s native byte order
                const auto packetSize = absl::big_endian::ToHost(*uint32DataPtr);
                // update buffer
                addData(targetBuffer, sampleData + avccHeaderByteSize(), packetSize);
                size_t bytesWritten = packetSize + annexBHeaderSize();
                bytesRemaining -= bytesWritten;
                sampleData += bytesWritten;
            }
            if (0UL == bytesRemaining) {
                return {};
            }
        }
        return COMPLETION_STATUS(kCMBlockBufferBadLengthParameterErr);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CMVideoFormat MacH264BitstreamParser::createVideoFormatH264(const std::vector<uint8_t>& sps,
                                                            const std::vector<uint8_t>& pps,
                                                            const std::vector<uint8_t>& spsext)
{
    if (!sps.empty() && !pps.empty()) {
        // Build the configuration records.
        std::vector<const uint8_t*> nalu_data_ptrs;
        std::vector<size_t> nalu_data_sizes;
        nalu_data_ptrs.reserve(3);
        nalu_data_sizes.reserve(3);
        nalu_data_ptrs.push_back(&sps.front());
        nalu_data_sizes.push_back(sps.size());
        if (!spsext.empty()) {
            nalu_data_ptrs.push_back(&spsext.front());
            nalu_data_sizes.push_back(spsext.size());
        }
        nalu_data_ptrs.push_back(&pps.front());
        nalu_data_sizes.push_back(pps.size());
        CMFormatDescriptionRef format;
        auto status = COMPLETION_STATUS(CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                                            nalu_data_ptrs.size(),     // parameter_set_count
                                                                                            &nalu_data_ptrs.front(),   // &parameter_set_pointers
                                                                                            &nalu_data_sizes.front(),  // &parameter_set_sizes
                                                                                            annexBHeaderSize(),         // nal_unit_header_length
                                                                                            &format));
        if (status) {
            return format;
        }
        return status;
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CMVideoFormat MacH264BitstreamParser::createVideoFormatH264(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& annexBBuffer)
{
    if (annexBBuffer) {
        using webrtc::H264::kSps;
        webrtc::AnnexBBufferReader reader(annexBBuffer->data(), annexBBuffer->size());
        if (reader.SeekToNextNaluOfType(kSps)) {
            static thread_local const uint8_t* paramSetPtrs[2] = {};
            static thread_local size_t paramSetSizes[2] = {};
            if (reader.ReadNalu(&paramSetPtrs[0], &paramSetSizes[0]) &&
                reader.ReadNalu(&paramSetPtrs[1], &paramSetSizes[1])) {
                CMFormatDescriptionRef format;
                auto status = COMPLETION_STATUS(CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                                                    2UL,
                                                                                                    paramSetPtrs,
                                                                                                    paramSetSizes,
                                                                                                    annexBHeaderSize(),
                                                                                                    &format));
                if (status) {
                    return format;
                }
                return status;
            }
        }
        return COMPLETION_STATUS(readErr);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

} // namespace LiveKitCpp
#endif
