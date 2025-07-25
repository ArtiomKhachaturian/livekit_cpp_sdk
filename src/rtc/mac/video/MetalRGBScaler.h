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
#pragma once // MetalScaler.h
#include <memory>

namespace LiveKitCpp
{

class MetalRGBScaler
{
    struct Impl;
public:
    MetalRGBScaler();
    ~MetalRGBScaler();
    bool valid();
    bool scale(const std::byte* src, int srcStride,
               int srcWidth, int srcHeight,
               std::byte* dst, int dstStride,
               int dstWidth, int dstHeight);
private:
    static std::unique_ptr<Impl> create();
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
