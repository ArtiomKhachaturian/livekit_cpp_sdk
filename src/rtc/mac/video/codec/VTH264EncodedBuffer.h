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
#pragma once // VTH264EncodedBuffer.h
#include "EncodedImageBuffer.h"
#include <CoreMedia/CMSampleBuffer.h> // for CMSampleBufferRef
#include <api/scoped_refptr.h>


namespace LiveKitCpp
{

class CFMemoryPool;
class MemoryBlock;

class VTH264EncodedBuffer : public EncodedImageBuffer
{
public:
    static MaybeEncodedImageBuffer create(const CFMemoryPool* memoryPool,
                                          CMSampleBufferRef sampleBuffer,
                                          bool isKeyFrame,
                                          size_t initialMemoryBlockSize);
private:
    static CompletionStatus addNaluForKeyFrame(bool isKeyFrame,
                                               MemoryBlock* annexbBuffer,
                                               CMSampleBufferRef sampleBuffer);
    static CompletionStatus storeAnnexBFrame(MemoryBlock* annexbBuffer,
                                             CMSampleBufferRef sampleBuffer);
};

} // namespace LiveKitCpp
