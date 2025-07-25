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
#pragma once // CapturedFrameReceiver.h
#include <atlbase.h> //CComPtr support
#include <stddef.h>
#include <strmif.h>

namespace webrtc {
struct VideoCaptureCapability;
}

namespace LiveKitCpp 
{

class CapturedFrameReceiver
{
public:
    virtual void deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                              const CComPtr<IMediaSample>& sample,
                              const webrtc::VideoCaptureCapability& frameInfo) = 0;

protected:
    virtual ~CapturedFrameReceiver() = default;
};

} // namespace LiveKitCpp