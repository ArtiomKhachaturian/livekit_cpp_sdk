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
#pragma once // MemoryBlock.h
#include "Blob.h"
#include <cassert>
#include <memory>

namespace LiveKitCpp
{

class MemoryBlock : public Bricks::Blob
{
public:
    ~MemoryBlock() override = default;
    size_t capacity() const;
    void setSize(size_t size);
    void ensureCapacity(size_t capacity);
    template <typename U>
    void appendData(const U* data, size_t size);
    template <typename U, size_t N>
    void appendData(const U (&array)[N]);
    template <typename U>
    void setData(const U* data, size_t size);
    template<typename U, size_t N>
    void setData(const U (&array)[N]) { setData<U>(array, N); }
    void clear();
    // overrides of Bricks::Blob
    size_t size() const final;
    uint8_t* data() final;
    const uint8_t* data() const final;
protected:
    MemoryBlock() = default;
    virtual uint8_t* reallocate(uint8_t* data, size_t newSize) = 0;
private:
    void ensureCapacityWithHeadroom(size_t capacity, bool extraHeadroom);
    bool consistent() const;
private:
    size_t _size = 0UL;
    size_t _capacity = 0UL;
    uint8_t* _data = nullptr;
};

template <typename U>
inline void MemoryBlock::appendData(const U* data, size_t size)
{
    if (data && size > 0UL) {
        assert(consistent());
        const auto newSize = _size + size;
        ensureCapacityWithHeadroom(newSize, true);
        std::memcpy(_data + _size, data, size);
        _size = newSize;
    }
}

template <typename U, size_t N>
inline void MemoryBlock::appendData(const U (&array)[N])
{
    appendData<U>(array, N);
}

template <typename U>
inline void MemoryBlock::setData(const U* data, size_t size)
{
    _size = 0UL;
    appendData<U>(data, size);
}

} // namespace LiveKitCpp
