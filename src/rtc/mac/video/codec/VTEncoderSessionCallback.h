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
#pragma once // VTEncoderSessionCallback.h
#ifdef USE_PLATFORM_ENCODERS
#include "VTEncoderSourceFrame.h"
#include <VideoToolbox/VTErrors.h>

namespace LiveKitCpp
{

class VTEncoderSessionCallback
{
public:
    virtual void onEncodedImage(VTEncoderSourceFrame frame,
                                VTEncodeInfoFlags infoFlags,
                                CMSampleBufferRef sampleBuffer) = 0;
    virtual void onError(CompletionStatus error, bool fatal) = 0;
protected:
    virtual ~VTEncoderSessionCallback() = default;
};

} // namespace LiveKitCpp
#endif
