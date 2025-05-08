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
#include "VTH264EncodedBuffer.h"
#ifndef USE_OPEN_H264_ENCODER
#include "MacH264BitstreamParser.h"
#include "CFMemoryBlock.h"
#include "CFMemoryPool.h"
#include "RtcUtils.h"
#import <VideoToolbox/VTErrors.h>

namespace LiveKitCpp
{

MaybeEncodedImageBuffer VTH264EncodedBuffer::create(const CFMemoryPool* memoryPool,
                                                    CMSampleBufferRef sampleBuffer,
                                                    bool isKeyFrame,
                                                    size_t initialMemoryBlockSize)
{
    if (sampleBuffer && memoryPool) {
        auto annexbBuffer = memoryPool->createMemoryBlock(initialMemoryBlockSize);
        auto status = addNaluForKeyFrame(isKeyFrame, annexbBuffer.get(), sampleBuffer);
        if (status) {
            status = storeAnnexBFrame(annexbBuffer.get(), sampleBuffer);
            if (status) {
                return EncodedImageBuffer::create(std::move(annexbBuffer));
            }
        }
        return status;
    }
    return COMPLETION_STATUS(kVTParameterErr);
}

CompletionStatus VTH264EncodedBuffer::addNaluForKeyFrame(bool isKeyFrame,
                                                         MemoryBlock* annexbBuffer,
                                                         CMSampleBufferRef sampleBuffer)
{
    if (annexbBuffer) {
        if (isKeyFrame) {
            return MacH264BitstreamParser::addNaluForKeyFrame(annexbBuffer, sampleBuffer);
        }
        return {};
    }
    return COMPLETION_STATUS(kVTParameterErr);
}

CompletionStatus VTH264EncodedBuffer::storeAnnexBFrame(MemoryBlock* annexbBuffer, CMSampleBufferRef sampleBuffer)
{
    return MacH264BitstreamParser::storeAnnexBFrame(annexbBuffer, sampleBuffer);
}

} // namespace LiveKitCpp
#endif
