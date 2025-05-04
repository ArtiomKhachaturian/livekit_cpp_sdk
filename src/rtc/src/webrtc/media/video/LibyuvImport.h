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
#pragma once // LibyuvImport.h
#include "livekit/rtc/media/VideoContentHint.h"
#include <common_video/libyuv/include/webrtc_libyuv.h>
#include <libyuv/convert.h>
#include <libyuv/convert_from.h>
#include <libyuv/convert_from_argb.h>
#include <libyuv/planar_functions.h>
#include <libyuv/rotate.h>
#include <libyuv/scale.h>
#include <libyuv/scale_argb.h>
#include <libyuv/scale_rgb.h>
#include <libyuv/scale_uv.h>
#include <libyuv/cpu_id.h>

namespace LiveKitCpp
{

inline libyuv::FilterMode mapLibYUV(VideoContentHint hint) {
    switch (hint) {
        case VideoContentHint::None:
            break;
        case VideoContentHint::Fluid:
            return libyuv::FilterMode::kFilterLinear;
        case VideoContentHint::Detailed:
            return libyuv::FilterMode::kFilterBilinear;
        case VideoContentHint::Text:
            return libyuv::FilterMode::kFilterBox;
    }
    return libyuv::FilterMode::kFilterNone;
}

} // namespace LiveKitCpp
