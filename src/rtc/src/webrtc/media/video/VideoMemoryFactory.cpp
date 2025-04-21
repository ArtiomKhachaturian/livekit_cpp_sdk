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
#include "VideoMemoryFactory.h"
#include <modules/desktop_capture/shared_memory.h>

namespace LiveKitCpp
{

webrtc::scoped_refptr<webrtc::I420Buffer> VideoMemoryFactory::allocateI420(int width, int height)
{
    return webrtc::I420Buffer::Create(width, height);
}

webrtc::scoped_refptr<webrtc::NV12Buffer> VideoMemoryFactory::allocateNV12(int width, int height)
{
    return webrtc::NV12Buffer::Create(width, height);
}

std::unique_ptr<webrtc::SharedMemory> VideoMemoryFactory::createSharedMemory(size_t size)
{
    return {};
}

} // namespace LiveKitCpp
