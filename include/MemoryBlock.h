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
#include <memory>
#include <vector>

namespace LiveKitCpp
{

// implementation maybe thread-safe (but not neccessary) & exceptions-free
class MemoryBlock
{
public:
    virtual ~MemoryBlock() = default;
    size_t size() const noexcept { return _size; }
    uint8_t* data() noexcept { return _data; }
    const uint8_t* data() const noexcept { return _data; }
    static std::shared_ptr<MemoryBlock> make(std::vector<uint8_t> data);
protected:
    MemoryBlock(uint8_t* data = nullptr, size_t size = 0U) noexcept;
    void setData(uint8_t* data, size_t size) noexcept;
private:
    MemoryBlock(const MemoryBlock&) = delete;
    MemoryBlock(MemoryBlock&&) = delete;
    MemoryBlock& operator = (const MemoryBlock&) = delete;
    MemoryBlock& operator = (MemoryBlock&&) = delete;
private:
    uint8_t* _data = nullptr;
    size_t _size = 0U;
};

} // namespace LiveKitCpp
