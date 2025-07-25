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
#include "MemoryBlock.h"
#include <algorithm>

namespace LiveKitCpp
{

size_t MemoryBlock::capacity() const
{
    assert(consistent());
    return _capacity;
}

void MemoryBlock::setSize(size_t size)
{
    if (size != _size) {
        // Sets the size of the buffer. If the new size is smaller than the old, the
        // buffer contents will be kept but truncated; if the new size is greater,
        // the existing contents will be kept and the new space will be
        // uninitialized.
        ensureCapacityWithHeadroom(size, true);
        _size = size;
    }
}

void MemoryBlock::ensureCapacity(size_t capacity)
{
    // ensure that the buffer size can be increased to at least capacity without further reallocation,
    // of course, this operation might need to reallocate the buffer
    //
    // don't allocate extra headroom, since the user is asking for a specific capacity
    ensureCapacityWithHeadroom(capacity, false);
}

void MemoryBlock::clear()
{
    if (_size > 0UL) {
        _size = 0UL;
        assert(consistent());
    }
}

size_t MemoryBlock::size() const
{
    assert(consistent());
    return _size;
}

uint8_t* MemoryBlock::data()
{
    assert(consistent());
    return _data;
}

const uint8_t* MemoryBlock::data() const
{
    assert(consistent());
    return _data;
}

void MemoryBlock::ensureCapacityWithHeadroom(size_t capacity, bool extraHeadroom)
{
    assert(consistent());
    if (capacity != _capacity) {
        // If the caller asks for extra headroom, ensure that the new capacity is
        // >= 1.5 times the old capacity. Any constant > 1 is sufficient to prevent
        // quadratic behavior; as to why we pick 1.5 in particular, see
        // https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md and
        // http://www.gahcep.com/cpp-internals-stl-vector-part-1/.
        if (capacity > _capacity) { // grow
            if (extraHeadroom) {
                capacity = std::max(capacity, _capacity + _capacity / 2UL);
            }
        }
        _data = reallocate(_data, capacity);
        _capacity = capacity;
        assert(consistent());
    }
}

bool MemoryBlock::consistent() const
{
    return (_data != nullptr || 0UL == _capacity) && _capacity >= _size;
}

} // namespace LiveKitCpp
