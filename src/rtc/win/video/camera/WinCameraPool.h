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
#pragma once // WinCameraPool.h
#include "VideoFrameBufferPool.h"
#include <api/scoped_refptr.h>
#include <memory>
#include <string>

namespace webrtc {
class VideoCaptureModule;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp 
{

class CameraCapturer;
struct MediaDeviceInfo;

class WinCameraPool
{
    class Impl;
    class ReleaseManager;
    class CameraWrapper;
public:
    static rtc::scoped_refptr<CameraCapturer>
        create(const MediaDeviceInfo& device, 
               VideoFrameBufferPool framesPool = {},
               const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    static const std::shared_ptr<Impl>& implementation();
};

} // namespace LiveKitCpp