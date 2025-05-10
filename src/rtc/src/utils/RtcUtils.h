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
#pragma once // Utils.h
#ifdef __APPLE__
#include "CFAutoRelease.h"
#include <CoreMedia/CMTime.h>
#endif // WEBRTC_MAC
#include <api/media_types.h>
#include <api/peer_connection_interface.h>
#include <api/task_queue/task_queue_base.h>
#include <modules/desktop_capture/desktop_capture_types.h>
#include <memory>
#include <optional>
#include <string>

#ifdef __APPLE__
#ifdef __OBJC__
@class NSString;
@class NSMutableDictionary;
@class NSNumber;
#else
typedef struct objc_object NSString;
typedef struct objc_object NSMutableDictionary;
typedef struct objc_object NSNumber;
#endif
#endif // __APPLE__

namespace LiveKitCpp
{

enum class DisconnectReason;
enum class LiveKitError;
enum class TrackType;

#ifdef __APPLE__
// return zero if failed
int64_t cmTimeToMicro(const CMTime& time);
int32_t cmTimeToMilli(const CMTime& time);
CFStringRefAutoRelease stringToCFString(std::string_view str);
std::string osStatusToString(OSStatus status);
void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, NSString* value);
void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, NSNumber* value);
void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, Boolean value);
void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, CFStringRef value);
#endif // __APPLE__

std::optional<LiveKitError> toLiveKitError(DisconnectReason reason);

TrackType mediaTypeToTrackType(webrtc::MediaType type);
std::string fourccToString(int fourcc);
std::string makeUuid();
// human readable string for reflect of changes for some types
std::string makeStateChangesString(webrtc::PeerConnectionInterface::PeerConnectionState from,
                                   webrtc::PeerConnectionInterface::PeerConnectionState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceConnectionState from,
                                   webrtc::PeerConnectionInterface::IceConnectionState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::SignalingState from,
                                   webrtc::PeerConnectionInterface::SignalingState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceGatheringState from,
                                   webrtc::PeerConnectionInterface::IceGatheringState to);
std::string makeStateChangesString(webrtc::TaskQueueBase::DelayPrecision from,
                                   webrtc::TaskQueueBase::DelayPrecision to);
// task queue helpers
std::unique_ptr<webrtc::TaskQueueFactory> createTaskQueueFactory(const webrtc::FieldTrialsView* fieldTrials = nullptr);
std::shared_ptr<webrtc::TaskQueueBase>
    createTaskQueueS(absl::string_view queueName = {},
                     webrtc::TaskQueueFactory::Priority priority = webrtc::TaskQueueFactory::Priority::LOW,
                     const webrtc::FieldTrialsView* fieldTrials = nullptr);
std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>
    createTaskQueueU(absl::string_view queueName = {},
                     webrtc::TaskQueueFactory::Priority priority = webrtc::TaskQueueFactory::Priority::LOW,
                     const webrtc::FieldTrialsView* fieldTrials = nullptr);

} // namespace LiveKitCpp
