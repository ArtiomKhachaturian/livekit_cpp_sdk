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

namespace {

class VectorMemoryBlock : public LiveKitCpp::MemoryBlock
{
public:
    VectorMemoryBlock(std::vector<uint8_t> data);
private:
    std::vector<uint8_t> _data;
};

}

namespace LiveKitCpp
{

MemoryBlock::MemoryBlock(uint8_t* data, size_t size) noexcept
{
    setData(data, size);
}

void MemoryBlock::setData(uint8_t* data, size_t size) noexcept
{
    _data = data;
    _size = size;
}

std::shared_ptr<MemoryBlock> MemoryBlock::make(std::vector<uint8_t> data)
{
    std::shared_ptr<MemoryBlock> block;
    if (!data.empty()) {
        // avoid of [std::make_shared<>()] for prevention of dangling memory blocks
        // with weak references ([std::weak_ptr<>])
        block.reset(new VectorMemoryBlock(std::move(data)));
    }
    return block;
}

} // namespace LiveKitCpp


namespace  {

VectorMemoryBlock::VectorMemoryBlock(std::vector<uint8_t> data)
    : _data(std::move(data))
{
    setData(_data.data(), _data.size());
}

}
