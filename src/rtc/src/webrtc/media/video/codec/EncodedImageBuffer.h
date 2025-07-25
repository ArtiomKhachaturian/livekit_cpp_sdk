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
#pragma once // EncodedImageBuffer.h
#include "CompletionStatusOr.h"
#include <api/video/encoded_image.h>
#include <rtc_base/buffer.h>
#include <memory>
#include <vector>

namespace Bricks {
class Blob;
}

namespace LiveKitCpp
{

class EncodedImageBuffer : public webrtc::EncodedImageBufferInterface
{
public:
    static webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> create(webrtc::Buffer buffer);
    static webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> create(std::vector<uint8_t> buffer);
    static webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> create(std::unique_ptr<Bricks::Blob> buffer);
};

using MaybeEncodedImageBuffer = CompletionStatusOrScopedRefPtr<webrtc::EncodedImageBufferInterface>;

} // namespace LiveKitCpp
