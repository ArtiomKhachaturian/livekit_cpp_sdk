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
#pragma once // VideoMemoryFactory.h
#include <api/video/i420_buffer.h>
#include <api/video/nv12_buffer.h>
#include <api/scoped_refptr.h>

namespace webrtc {
class SharedMemory;
}

namespace LiveKitCpp
{

class VideoMemoryFactory
{
public:
    static webrtc::scoped_refptr<webrtc::I420Buffer> allocateI420(int width, int height);
    static webrtc::scoped_refptr<webrtc::NV12Buffer> allocateNV12(int width, int height);
    static std::unique_ptr<webrtc::SharedMemory> createSharedMemory(size_t size);
};
	
} // namespace LiveKitCpp
