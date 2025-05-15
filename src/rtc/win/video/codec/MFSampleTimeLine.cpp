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
#include "MFSampleTimeLine.h"
#include "RtcUtils.h"
#include <api/video/encoded_image.h>
#include <api/video/video_frame.h>
#include <mfapi.h>
#include <rtc_base/time_utils.h>

namespace LiveKitCpp 
{

MFSampleTimeLine::MFSampleTimeLine(bool roundTo90kHz)
    : _roundTo90kHz(roundTo90kHz)
{
}

CompletionStatus MFSampleTimeLine::setTimeMetrics(LONGLONG timestampMicro, IMFSample* sample)
{
    LONGLONG timestampHns = 0LL;
    if (0LL == _startTimeMicro) {
        _startTimeMicro = timestampMicro;
    }
    else {
        timestampHns = (timestampMicro - _startTimeMicro) * _oneMicrosecondInMFSampleTimeUnits;
    }
    CompletionStatus hr;
    if (sample) {
        hr = COMPLETION_STATUS(sample->SetSampleTime(timestampHns));
        if (setupDuration() && hr) {
            const auto duration = timestampHns - _lastTimestampHns;
            hr = COMPLETION_STATUS(sample->SetSampleDuration(duration));
        }
    }
    if (hr.ok()) {
        _lastDurationHns = timestampHns - _lastTimestampHns;
        _lastTimestampHns = timestampHns;
    }
    return hr;
}

CompletionStatus MFSampleTimeLine::setTimeMetrics(const webrtc::EncodedImage& from, IMFSample* sample)
{
    return setTimeMetrics(timestampMicro(from), sample);
}

CompletionStatus MFSampleTimeLine::setTimeMetrics(const webrtc::VideoFrame& from, IMFSample* sample)
{
    return setTimeMetrics(timestampMicro(from), sample);
}

LONGLONG MFSampleTimeLine::timestampMicro(const webrtc::EncodedImage& from) const
{
    // this is RTP timestamp (ms), already rounded to 90khz
    return from.RtpTimestamp() * rtc::kNumMicrosecsPerMillisec;
}

LONGLONG MFSampleTimeLine::timestampMicro(const webrtc::VideoFrame& from) const
{
    if (const auto rtpTimestampMs = from.rtp_timestamp()) { // already rounded to 90khz
        return rtpTimestampMs * rtc::kNumMicrosecsPerMillisec;
    }
    if (auto sysTimestampMicro = from.timestamp_us()) {
        if (_roundTo90kHz) {
            sysTimestampMicro = rounded90kHz(sysTimestampMicro);
        }
        return sysTimestampMicro;
    }
    return 0LL;
}

} // namespace LiveKitCpp