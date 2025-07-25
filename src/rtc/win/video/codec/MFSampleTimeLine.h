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
#pragma once // MFSampleTimeLine.h
#include "CompletionStatus.h"
#include <mfobjects.h>

namespace webrtc {
class EncodedImage;
class VideoFrame;
} // namespace webrtc

namespace LiveKitCpp 
{

class MFSampleTimeLine
{
public:
    MFSampleTimeLine(bool roundTo90kHz = true);
    void reset() { _startTimeMicro = _lastDurationHns = _lastTimestampHns = 0LL; }
    void setSetupDuration(bool set) { _setupDuration = set; }
    bool setupDuration() const { return _setupDuration; }
    LONGLONG lastTimestampHns() const noexcept { return _lastTimestampHns; }
    LONGLONG lastDurationHns() const noexcept { return _lastDurationHns; }
    CompletionStatus setTimeMetrics(LONGLONG timestampMicro, IMFSample* sample = nullptr);
    CompletionStatus setTimeMetrics(const webrtc::EncodedImage& from, IMFSample* sample = nullptr);
    CompletionStatus setTimeMetrics(const webrtc::VideoFrame& from, IMFSample* sample = nullptr);
private:
    template <typename T>
    static T rounded90kHz(T value) { return 100 * (value / 90.0 + 0.5f); }
    LONGLONG timestampMicro(const webrtc::EncodedImage& from) const;
    LONGLONG timestampMicro(const webrtc::VideoFrame& from) const;
private:
    // Media Foundation uses 100 nanosecond units for time, see
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms697282(v=vs.85).aspx.
    static inline constexpr const int8_t _oneMicrosecondInMFSampleTimeUnits = 10;
    // required for some formats, for example H.264 clock rate is 90kHz (https://tools.ietf.org/html/rfc6184#page-11)
    bool _roundTo90kHz;
    LONGLONG _startTimeMicro = 0LL;
    LONGLONG _lastTimestampHns = 0LL;
    LONGLONG _lastDurationHns = 0LL;
    bool _setupDuration = true;
};

} // namespace LiveKitCpp