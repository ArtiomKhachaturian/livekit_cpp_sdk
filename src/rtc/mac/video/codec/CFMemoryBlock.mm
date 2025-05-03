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
#include "CFMemoryBlock.h"
#include "CFMemoryPool.h"
#include "CFAutoRelease.h"

namespace LiveKitCpp
{

class CFMemoryBlock::InplaceAllocatorSource : public CFAllocatorSource
{
public:
    InplaceAllocatorSource(CFAllocatorRef allocator);
    ~InplaceAllocatorSource() override = default;
    static std::shared_ptr<const CFAllocatorSource> create(CFAllocatorRef allocator, bool retain);
    // impl. of CFAllocatorSource
    CFAllocatorRef allocator() const final { return _allocator; }
private:
    const CFAutoRelease<CFAllocatorRef> _allocator;
};

CFMemoryBlock::CFMemoryBlock(const std::shared_ptr<const CFAllocatorSource>& allocatorSource,
                             size_t initialCapacity)
    : _allocatorSource(allocatorSource)
{
    if (initialCapacity > 0U) {
        ensureCapacity(initialCapacity);
    }
}

CFMemoryBlock::CFMemoryBlock(CFAllocatorRef allocator, bool retainAllocator,
                             size_t initialCapacity)
    : CFMemoryBlock(InplaceAllocatorSource::create(allocator, retainAllocator), initialCapacity)
{
}

CFMemoryBlock::~CFMemoryBlock()
{
    if (const auto a = allocator()) {
        if (const auto allocated = data()) {
            CFAllocatorDeallocate(a, allocated);
        }
    }
}

uint8_t* CFMemoryBlock::reallocate(uint8_t* data, size_t newSize)
{
    if (const auto a = allocator()) {
        return reinterpret_cast<uint8_t*>(CFAllocatorReallocate(a, data, newSize, 0));
    }
    return nullptr;
}

CFAllocatorRef CFMemoryBlock::allocator() const
{
    if (_allocatorSource) {
        return _allocatorSource->allocator();
    }
    return {};
}

CFMemoryBlock::InplaceAllocatorSource::InplaceAllocatorSource(CFAllocatorRef allocator)
    : _allocator(allocator)
{
}

std::shared_ptr<const CFAllocatorSource> CFMemoryBlock::InplaceAllocatorSource::create(CFAllocatorRef allocator,
                                                                                       bool retain)
{
    if (allocator) {
        if (retain) {
            allocator = reinterpret_cast<CFAllocatorRef>(CFRetain(allocator));
        }
        return std::make_shared<InplaceAllocatorSource>(allocator);
    }
    return nullptr;
}

} // namespace LiveKitCpp
