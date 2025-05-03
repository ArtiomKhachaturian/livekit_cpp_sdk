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
#include "CFMemoryPool.h"
#include "CFMemoryBlock.h"

namespace LiveKitCpp
{

CFMemoryPool::CFMemoryPool(CFAutoRelease<CMMemoryPoolRef> impl, CFAllocatorRef allocator)
    : _impl(std::move(impl))
    , _allocator(allocator)
{
}

CFMemoryPool::~CFMemoryPool()
{
    if (_impl) {
        CMMemoryPoolFlush(_impl);
        CMMemoryPoolInvalidate(_impl);
    }
}

std::shared_ptr<CFMemoryPool> CFMemoryPool::create(CFDictionaryRef options)
{
    std::shared_ptr<CFMemoryPool> pool;
    if (CFAutoRelease<CMMemoryPoolRef> impl = CMMemoryPoolCreate(options)) {
        if (auto allocator = CMMemoryPoolGetAllocator(impl)) {
            pool.reset(new CFMemoryPool(std::move(impl), allocator));
        }
    }
    return pool;
}

std::unique_ptr<CFMemoryBlock> CFMemoryPool::createMemoryBlock(size_t initialCapacity) const
{
    auto block = std::make_unique<CFMemoryBlock>(shared_from_this(), initialCapacity);
    if (block->valid()) {
        return block;
    }
    return {};
}

} // namespace LiveKitCpp
