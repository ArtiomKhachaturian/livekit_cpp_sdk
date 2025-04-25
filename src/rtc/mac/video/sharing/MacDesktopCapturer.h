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
#pragma once // MacDesktopCapturer.h
#include "DesktopCapturer.h"
#include <api/scoped_refptr.h>
#include <optional>

namespace LiveKitCpp
{

class MacDesktopCapturer : public DesktopCapturer
{
public:
    ~MacDesktopCapturer() override;
protected:
    MacDesktopCapturer(bool window, bool previewMode,
                       webrtc::DesktopCaptureOptions options,
                       VideoFrameBufferPool framesPool = {});
    std::unique_ptr<webrtc::DesktopFrame> captureDisplay(webrtc::ScreenId sId) const;
    std::unique_ptr<webrtc::DesktopFrame> captureWindow(webrtc::WindowId wId) const;
};


} // namespace LiveKitCpp
