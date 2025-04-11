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
#include "Utils.h"
#include "livekit/rtc/LiveKitError.h"
#include "livekit/signaling/sfu/DisconnectReason.h"
#include "livekit/signaling/sfu/TrackType.h"
#ifndef _WIN32
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#endif
#include <api/task_queue/default_task_queue_factory.h>
#include <rtc_base/crypto_random.h>
#include <thread>
#include <codecvt>
#include <locale>

using ConvertType = std::codecvt_utf8<wchar_t>;

namespace LiveKitCpp
{

std::string fromWideChar(const std::wstring& w)
{
    if (!w.empty()) {
        std::wstring_convert<ConvertType, wchar_t> converter;
        return converter.to_bytes(w);
    }
    return {};
}

bool compareCaseInsensitive(std::string_view s1, std::string_view s2)
{
    const size_t size = s1.size();
    if (size == s2.size()) {
        for (size_t i = 0U; i < size; ++i) {
            if (std::tolower(s1[i]) != std::tolower(s2[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

std::vector<std::string> split(std::string_view s, std::string_view delim)
{
    std::vector<std::string> res;
    if (!s.empty() && !delim.empty()) {
        std::string::size_type pos, lastPos = 0U, length = s.length();
        while (lastPos < length + 1U) {
            pos = s.find_first_of(delim, lastPos);
            if (pos == std::string::npos) {
                pos = length;
            }
            if (pos != lastPos)
                res.emplace_back(s.data() + lastPos, pos - lastPos);

            lastPos = pos + 1U;
        }
    }
    return res;
}

std::string join(const std::vector<std::string>& strings,
                 std::string_view delim, bool skipEmpty)
{
    if (!strings.empty()) {
        const std::vector<std::string_view> stringViews(strings.begin(), strings.end());
        return join(stringViews, delim, skipEmpty);
    }
    return std::string();
}

std::string join(const std::vector<std::string_view>& strings,
                 std::string_view delim, bool skipEmpty)
{
    if (!strings.empty()) {
        const auto size = strings.size();
        if (size > 1U) {
            std::string result;
            for (size_t i = 0U, last = size - 1U; i < size; ++i) {
                const auto& string = strings.at(i);
                if (!skipEmpty || !string.empty()) {
                    result += string;
                    if (i != last) {
                        result += delim;
                    }
                }
            }
            return result;
        }
        return std::string(strings.front().data(), strings.front().size());
    }
    return std::string();
}

template <typename TPCEnum>
inline std::string stateToString(TPCEnum state) { return {}; }

template <typename TPCEnum>
inline std::string enumTypeToString() { return {}; }

template <typename TPCEnum>
inline std::string makeChangesString(TPCEnum from, TPCEnum to) {
    return enumTypeToString<TPCEnum>() + " state changed from '" +
           stateToString(from) + "' to '" + stateToString(to) + "'";
}

template <>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::PeerConnectionState>() {
    return "peer connection";
}

template <>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::IceConnectionState>() {
    return "ICE connection";
}

template <>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::SignalingState>() {
    return "signaling";
}

template <>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::IceGatheringState>() {
    return "ICE gathering";
}

template <>
inline std::string enumTypeToString<webrtc::TaskQueueBase::DelayPrecision>() {
    return "delay precision";
}

inline std::string fromAbsStringView(absl::string_view s) {
    return std::string(s.data(), s.size());
}

template <>
inline std::string stateToString<webrtc::PeerConnectionInterface::PeerConnectionState>(webrtc::PeerConnectionInterface::PeerConnectionState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template <>
inline std::string stateToString<webrtc::PeerConnectionInterface::IceConnectionState>(webrtc::PeerConnectionInterface::IceConnectionState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template <>
inline std::string stateToString<webrtc::PeerConnectionInterface::SignalingState>(webrtc::PeerConnectionInterface::SignalingState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template <>
inline std::string stateToString<webrtc::PeerConnectionInterface::IceGatheringState>(webrtc::PeerConnectionInterface::IceGatheringState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template <webrtc::TaskQueueBase::DelayPrecision>
inline std::string stateToString(webrtc::TaskQueueBase::DelayPrecision precision) {
    switch (precision) {
        case webrtc::TaskQueueBase::DelayPrecision::kLow:
            return "low";
        case webrtc::TaskQueueBase::DelayPrecision::kHigh:
            return "high";
        default:
            assert(false);
            break;
    }
    return {};
}

TrackType mediaTypeToTrackType(cricket::MediaType type)
{
    switch (type) {
        case cricket::MEDIA_TYPE_AUDIO:
            return TrackType::Audio;
        case cricket::MEDIA_TYPE_VIDEO:
            return TrackType::Video;
        case cricket::MEDIA_TYPE_DATA:
            break;
        default:
            assert(false);
            break;
    }
    return TrackType::Data;
}

std::string fourccToString(int fourcc)
{
    const auto fourccStr = (const char*)&fourcc;
    return std::string(fourccStr, 4U);
}

std::string makeUuid()
{
    return rtc::CreateRandomUuid();
}

std::string makeStateChangesString(webrtc::PeerConnectionInterface::PeerConnectionState from,
                                   webrtc::PeerConnectionInterface::PeerConnectionState to)
{
    return makeChangesString(from, to);
}

std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceConnectionState from,
                                   webrtc::PeerConnectionInterface::IceConnectionState to)
{
    return makeChangesString(from, to);
}

std::string makeStateChangesString(webrtc::PeerConnectionInterface::SignalingState from,
                                   webrtc::PeerConnectionInterface::SignalingState to)
{
    return makeChangesString(from, to);
}

std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceGatheringState from,
                                   webrtc::PeerConnectionInterface::IceGatheringState to)
{
    return makeChangesString(from, to);
}

std::string makeStateChangesString(webrtc::TaskQueueBase::DelayPrecision from,
                                   webrtc::TaskQueueBase::DelayPrecision to)
{
    return makeChangesString(from, to);
}

std::unique_ptr<webrtc::TaskQueueFactory> createTaskQueueFactory(const webrtc::FieldTrialsView* fieldTrials)
{
    return webrtc::CreateDefaultTaskQueueFactory(fieldTrials);
}

std::shared_ptr<webrtc::TaskQueueBase>
    createTaskQueueS(absl::string_view queueName,
                     webrtc::TaskQueueFactory::Priority priority,
                     const webrtc::FieldTrialsView* fieldTrials)
{
    if (auto queue = createTaskQueueU(queueName, priority, fieldTrials)) {
        return std::shared_ptr<webrtc::TaskQueueBase>(queue.release(), [](webrtc::TaskQueueBase* q) {
            if (q) {
                std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> owned(q);
                if (owned->IsCurrent()) {
                    std::thread([owned = std::move(owned)]() mutable {
                        owned.reset(); }).detach();
                }
                else {
                    owned.reset();
                }
            }
        });
    }
    return {};
}

std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>
    createTaskQueueU(absl::string_view queueName,
                     webrtc::TaskQueueFactory::Priority priority,
                     const webrtc::FieldTrialsView* fieldTrials)
{
    if (auto factory = createTaskQueueFactory(fieldTrials)) {
#ifdef WIN32
        // Win32 has limitation for thread name - max 63 symbols
        if (queueName.size() > 62U) {
            queueName = queueName.substr(0, 62U);
        }
#endif
        return factory->CreateTaskQueue(queueName, priority);
    }
    return {};
}

std::optional<LiveKitError> toLiveKitError(DisconnectReason reason)
{
    switch (reason) {
        case DisconnectReason::DuplicateIdentity:
            return LiveKitError::ServerDuplicateIdentity;
        case DisconnectReason::ServerShutdown:
            return LiveKitError::ServerShutdown;
        case DisconnectReason::ParticipantRemoved:
            return LiveKitError::ServerParticipantRemoved;
        case DisconnectReason::RoomDeleted:
            return LiveKitError::ServerRoomDeleted;
        case DisconnectReason::StateMismatch:
            return LiveKitError::ServerStateMismatch;
        case DisconnectReason::JoinFailure:
            return LiveKitError::ServerJoinFailure;
        case DisconnectReason::Migration:
            return LiveKitError::ServerMigration;
        case DisconnectReason::SignalClose:
            return LiveKitError::ServerSignalClose;
        case DisconnectReason::RoomClosed:
            return LiveKitError::ServerRoomClosed;
        case DisconnectReason::UserUnavailable:
            return LiveKitError::ServerUserUnavailable;
        case DisconnectReason::UserRejected:
            return LiveKitError::ServerUserRejected;
        case DisconnectReason::SipTrunkFailure:
            return LiveKitError::ServerSipTrunkFailure;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
