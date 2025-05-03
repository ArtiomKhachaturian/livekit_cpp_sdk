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
#pragma once // CFMemoryBlock.h
#include "MemoryBlock.h"
#include <CoreFoundation/CFBase.h>

namespace LiveKitCpp
{

class CFAllocatorSource;
class CFMemoryPool;

class CFMemoryBlock : public MemoryBlock
{
    class InplaceAllocatorSource;
public:
    CFMemoryBlock(const std::shared_ptr<const CFAllocatorSource>& allocatorSource,
                  size_t initialCapacity = 0UL);
    CFMemoryBlock(CFAllocatorRef allocator, bool retainAllocator,
                  size_t initialCapacity = 0UL);
    ~CFMemoryBlock() final;
    bool valid() const { return nullptr != allocator(); }
protected:
    // impl. of MemoryBlock
    uint8_t* reallocate(uint8_t* data, size_t newSize) final;
private:
    CFAllocatorRef allocator() const;
private:
    const std::shared_ptr<const CFAllocatorSource> _allocatorSource;
};

} // namespace LiveKitCpp
