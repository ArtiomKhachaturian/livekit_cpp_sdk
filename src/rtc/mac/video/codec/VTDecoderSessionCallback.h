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
#pragma once // VTDecoderSessionCallback.h
#ifdef USE_PLATFORM_DECODERS
#include "CompletionStatus.h"
#include <CoreMedia/CMTime.h>
#include <api/scoped_refptr.h>
#include <api/video/video_frame_buffer.h>
#include <api/video/color_space.h>
#include <optional>
#include <VideoToolbox/VTErrors.h>

namespace webrtc {
class ColorSpace;
}

namespace LiveKitCpp
{

class VTDecoderSessionCallback
{
public:
    // buffer can be a null_ptr if frame was dropped by decoder, check [infoFlags]
    virtual void onDecodedImage(CMTime timestamp, CMTime duration,
                                VTDecodeInfoFlags infoFlags,
                                uint32_t encodedImageTimestamp,
                                webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                                std::optional<uint8_t> qp = std::nullopt,
                                std::optional<webrtc::ColorSpace> encodedImageColorspace = std::nullopt) = 0;
    virtual void onError(CompletionStatus error, bool fatal) = 0;
protected:
    virtual ~VTDecoderSessionCallback() = default;
};

} // namespace LiveKitCpp
#endif
