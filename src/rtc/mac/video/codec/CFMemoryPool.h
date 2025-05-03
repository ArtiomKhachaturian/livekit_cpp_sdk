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
#pragma once // CFMemoryPool.h
#include "CFAutoRelease.h"
#include "CFAllocatorSource.h"
#include <CoreMedia/CMMemoryPool.h>
#include <memory>

namespace LiveKitCpp
{

class CFMemoryBlock;

class CFMemoryPool : public CFAllocatorSource,
                     public std::enable_shared_from_this<CFMemoryPool>
{
public:
    ~CFMemoryPool() final;
    CMMemoryPoolRef pool() const { return _impl; }
    // impl. of CFAllocatorSource
    CFAllocatorRef allocator() const final { return _allocator; }
    static std::shared_ptr<CFMemoryPool> create(CFDictionaryRef options = nullptr);
    std::unique_ptr<CFMemoryBlock> createMemoryBlock(size_t initialCapacity = 0U) const;
private:
    CFMemoryPool(CFAutoRelease<CMMemoryPoolRef> impl, CFAllocatorRef allocator);
private:
    const CFAutoRelease<CMMemoryPoolRef> _impl;
    const CFAllocatorRef _allocator;
};

} // namespace LiveKitCpp
