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
#include "WebRtcLogSink.h"
//#include <QtLogging>
//#include <QDebug>
#include <algorithm>

namespace {

const char* g_ignoreWebRtcLogLabels[] = {
    "Blocking GetStats: ",
    "Duplicate id found. Reassigning",
    "Not supported on this platform",
    "RED codec red is missing an associated payload type",
    "Task queue scheduled delayed call too early",
    "Ignore it",
    "Trying to call unset encoder",
    "both expected greater or equal to 0",
#ifdef WEBRTC_MAC
    "Allow IOSurface: ",
#endif
};

inline bool ignoreWebRtcLogString(const std::string& string)
{
    if (string.size() > 1UL) {
        for (const auto& label : g_ignoreWebRtcLogLabels) {
            if (std::string::npos != string.find(label)) {
                return true;
            }
        }
        return false;
    }
    return true;
}

inline const std::string& alignedWebRtcLogString(const std::string& string)
{
    if (!string.empty()) {
        const auto hasLFAtTheEnd = '\n' == string.back();
        const auto posLfInMiddle = string.find_first_of('\n');
        const auto hasLfInMiddle = std::string::npos != posLfInMiddle && posLfInMiddle != string.size() - 1;
        if (hasLFAtTheEnd || hasLfInMiddle) {
            static thread_local std::string alignedString;
            if (hasLFAtTheEnd) {
                alignedString.assign(string.c_str(), string.size() - 1UL);
            }
            if (!hasLFAtTheEnd) {
                alignedString = string;
            }
            if (hasLfInMiddle) {
                std::replace(alignedString.begin(), alignedString.end(), '\n', '\t');
            }
            return alignedString;
        }
    }
    return string;
}

} // namespace

namespace LiveKitCpp
{

WebRtcLogSink::WebRtcLogSink()
{
    rtc::LogMessage::AddLogToStream(this, rtc::LoggingSeverity::LS_VERBOSE);
}

WebRtcLogSink::~WebRtcLogSink()
{
    rtc::LogMessage::RemoveLogToStream(this);
}

void WebRtcLogSink::OnLogMessage(const std::string& message,
                                 rtc::LoggingSeverity severity)
{
    if (!ignoreWebRtcLogString(message)) {
        switch (severity) {
            case rtc::LS_VERBOSE:
                //qDebug() << QString::fromStdString(alignedWebRtcLogString(message));
                break;
            case rtc::LS_INFO:
                //qInfo() << QString::fromStdString(alignedWebRtcLogString(message));
                break;
            case rtc::LS_WARNING:
                //qWarning() << QString::fromStdString(alignedWebRtcLogString(message));
                break;
            case rtc::LS_ERROR:
                //qCritical() << QString::fromStdString(alignedWebRtcLogString(message));
                break;
            default:
                break;
        }
    }
}

void WebRtcLogSink::OnLogMessage(const std::string& message)
{
}

void WebRtcLogSink::OnLogMessage(absl::string_view message,
                                 rtc::LoggingSeverity severity)
{
}

void WebRtcLogSink::OnLogMessage(absl::string_view message)
{
}

} // namespace LiveKitCpp
