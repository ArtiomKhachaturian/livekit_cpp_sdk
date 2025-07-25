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
#pragma once // ServiceInitInfo.h
#include <optional>
#include <memory>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

struct ServiceInitInfo
{
    std::shared_ptr<Bricks::Logger> _logger;
    bool _logWebrtcEvents = false;
    // https://datatracker.ietf.org/doc/html/draft-ietf-payload-flexible-fec-scheme-03#section-7
    bool _enableFlexFec = false; // WebRTC-FlexFEC-03 + WebRTC-FlexFEC-03-Advertised
    // true if RED (Redundant Encoding) is disabled for audio
    std::optional<bool> _disableAudioRed;
    // https://jmvalin.ca/demo/rnnoise/
    bool _enableRNNoiseSuppressor = true;
};

} // namespace LiveKitCpp
