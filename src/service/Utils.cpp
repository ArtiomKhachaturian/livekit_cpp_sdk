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
#include "TransportState.h"
#include "NetworkType.h"
#ifndef _WIN32
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#endif
#ifdef WEBRTC_AVAILABLE
#include <api/task_queue/default_task_queue_factory.h>
#include <rtc_base/crypto_random.h>
#include <thread>
#endif
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

template<typename TPCEnum>
inline std::string stateToString(TPCEnum state) { return {}; }

template<typename TPCEnum>
inline std::string enumTypeToString() { return {}; }

template<>
inline std::string stateToString<TransportState>(TransportState state) {
    return toString(state);
}

template<>
inline std::string enumTypeToString<TransportState>() {
    return "transport";
}

template<typename TPCEnum>
inline std::string makeChangesString(TPCEnum from, TPCEnum to) {
    return enumTypeToString<TPCEnum>() + " state changed from '" +
           stateToString(from) + "' to '" + stateToString(to) + "'";
}

#ifdef WEBRTC_AVAILABLE
template<>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::PeerConnectionState>() {
    return "peer connection";
}

template<>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::IceConnectionState>() {
    return "ICE connection";
}

template<>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::SignalingState>() {
    return "signaling";
}

template<>
inline std::string enumTypeToString<webrtc::PeerConnectionInterface::IceGatheringState>() {
    return "ICE gathering";
}

template<>
inline std::string enumTypeToString<webrtc::TaskQueueBase::DelayPrecision>() {
    return "delay precision";
}

inline std::string fromAbsStringView(absl::string_view s) {
    return std::string(s.data(), s.size());
}

template<>
inline std::string stateToString<webrtc::PeerConnectionInterface::PeerConnectionState>(webrtc::PeerConnectionInterface::PeerConnectionState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template<>
inline std::string stateToString<webrtc::PeerConnectionInterface::IceConnectionState>(webrtc::PeerConnectionInterface::IceConnectionState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template<>
inline std::string stateToString<webrtc::PeerConnectionInterface::SignalingState>(webrtc::PeerConnectionInterface::SignalingState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template<>
inline std::string stateToString<webrtc::PeerConnectionInterface::IceGatheringState>(webrtc::PeerConnectionInterface::IceGatheringState state) {
    return fromAbsStringView(webrtc::PeerConnectionInterface::AsString(state));
}

template<webrtc::TaskQueueBase::DelayPrecision>
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

std::shared_ptr<webrtc::TaskQueueBase> createTaskQueue(webrtc::TaskQueueFactory::Priority priority,
                                                       absl::string_view queueName,
                                                       const webrtc::FieldTrialsView* fieldTrials)
{
#ifdef WIN32
    // Win32 has limitation for thread name - max 63 symbols
    if (queueName.size() > 62U) {
        queueName = queueName.substr(0, 62U);
    }
#endif
    if (auto factory = webrtc::CreateDefaultTaskQueueFactory(fieldTrials)) {
        using QueueUPtr = std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>;
        if (QueueUPtr queue = factory->CreateTaskQueue(queueName, priority)) {
            return std::shared_ptr<webrtc::TaskQueueBase>(queue.release(), [](webrtc::TaskQueueBase* q) {
                if (q) {
                    QueueUPtr owned(q);
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
    }
    return {};
}

#endif

std::string makeStateChangesString(TransportState from, TransportState to)
{
    return makeChangesString(from, to);
}

std::string toString(NetworkType state)
{
    switch (state) {
        case NetworkType::WiFi:
            return "wifi";
        case NetworkType::Wired:
            return "wired";
        case NetworkType::Cellular:
            return "cellular";
        case NetworkType::Vpn:
            return "vpn";
        default:
            break;
    }
    return "";
}

} // namespace LiveKitCpp
