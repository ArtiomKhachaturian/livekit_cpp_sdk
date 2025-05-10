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
#pragma once // AudioRecordingOptions.h
#include <optional>
#include <tuple>

namespace LiveKitCpp
{

struct AudioRecordingOptions
{
    // audio processing that attempts to filter away
    // the output signal from later inbound pickup.
    std::optional<bool> _echoCancellation;
    // audio processing to adjust the sensitivity of the local mic dynamically.
    std::optional<bool> _autoGainControl;
    // audio processing to filter out background noise.
    std::optional<bool> _noiseSuppression;
    // audio processing to remove background noise of lower frequencies.
    std::optional<bool> _highpassFilter;
    // audio processing to swap the left and right channels.
    std::optional<bool> _stereoSwapping;
};

} // namespace LiveKitCpp

inline bool operator == (const LiveKitCpp::AudioRecordingOptions& l, const LiveKitCpp::AudioRecordingOptions& r) {
    return &l == &r || (std::tie(l._autoGainControl, l._echoCancellation, l._highpassFilter, l._noiseSuppression, l._stereoSwapping) ==
                        std::tie(r._autoGainControl, r._echoCancellation, r._highpassFilter, r._noiseSuppression, r._stereoSwapping));
}

inline bool operator != (const LiveKitCpp::AudioRecordingOptions& l, const LiveKitCpp::AudioRecordingOptions& r) {
    return std::tie(l._autoGainControl, l._echoCancellation, l._highpassFilter, l._noiseSuppression, l._stereoSwapping) !=
           std::tie(r._autoGainControl, r._echoCancellation, r._highpassFilter, r._noiseSuppression, r._stereoSwapping);
}
