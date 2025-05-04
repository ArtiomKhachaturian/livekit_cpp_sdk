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
#pragma once // VTVideoEncoderFactory.h
#include "VideoEncoderFactory.h"

namespace LiveKitCpp
{

class VTVideoEncoderFactory : public VideoEncoderFactory
{
public:
    VTVideoEncoderFactory() = default;
    // impl. of VideoEncoderFactory
    CodecSupport QueryCodecSupport(const webrtc::SdpVideoFormat& format,
                                   std::optional<std::string> scalabilityMode) const override;
    std::unique_ptr<webrtc::VideoEncoder> Create(const webrtc::Environment& env,
                                                 const webrtc::SdpVideoFormat& format) override;
};

} // namespace LiveKitCpp
