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
#include "MediaAuthorization.h"
#include "MediaAuthorizationCallback.h"
#include "Loggable.h"
#include <atomic>
#include <thread>

namespace
{

using namespace LiveKitCpp;

std::atomic<MediaAuthorizationLevel> g_mediaAuthorizationLevel(MediaAuthorizationLevel::CheckOnly);

const std::string_view g_category = "media_authorization";

class SyncCallback : public MediaAuthorizationCallback
{
public:
    SyncCallback() = default;
    MediaAuthorizationStatus waitStatus();
    // impl. of MediaAuthorizationCallback
    void completed(MediaAuthorizationStatus status) final;
private:
    std::atomic<MediaAuthorizationStatus> _status = MediaAuthorizationStatus::NotDetermined;
    std::atomic_bool _completed = false;
};

class LoggedCallback : public Bricks::LoggableS<MediaAuthorizationCallback>
{
public:
    LoggedCallback(MediaAuthorizationKind kind, const std::shared_ptr<Bricks::Logger>& logger);
    // impl. of MediaAuthorizationCallback
    void completed(MediaAuthorizationStatus status) final;
private:
    const MediaAuthorizationKind _kind;
};

class NullCallback : public MediaAuthorizationCallback
{
public:
    NullCallback() = default;
    // impl. of MediaAuthorizationCallback
    void completed(MediaAuthorizationStatus) final {}
};

void logAuthorizationStatus(MediaAuthorizationKind kind,
                            MediaAuthorizationStatus status,
                            const std::shared_ptr<Bricks::Logger>& logger);


} // namespace

namespace LiveKitCpp
{

MediaAuthorizationStatus MediaAuthorization::request(MediaAuthorizationKind kind,
                                                     bool askPermissions,
                                                     const std::shared_ptr<Bricks::Logger>& logger)
{
    MediaAuthorizationStatus status = MediaAuthorizationStatus::Granted;
    const auto level = mediaAuthorizationLevel();
    if (MediaAuthorizationLevel::Disabled != level) {
        const auto callback = std::make_shared<SyncCallback>();
        if (MediaAuthorizationLevel::CheckOnly == level) {
            askPermissions = false;
        }
        requestFromSystem(kind, askPermissions, callback);
        status = callback->waitStatus();
    }
    logAuthorizationStatus(kind, status, logger);
    return status;
}

void MediaAuthorization::query(MediaAuthorizationKind kind,
                               bool askPermissions,
                               const std::shared_ptr<MediaAuthorizationCallback>& callback)
{
    if (callback) {
        const auto level = mediaAuthorizationLevel();
        if (MediaAuthorizationLevel::Disabled != level) {
            if (MediaAuthorizationLevel::CheckOnly == level) {
                askPermissions = false;
            }
            requestFromSystem(kind, askPermissions, callback);
        } else {
            callback->completed(MediaAuthorizationStatus::Granted);
        }
    }
}

void MediaAuthorization::query(MediaAuthorizationKind kind, bool askPermissions,
                                 const std::shared_ptr<Bricks::Logger>& logger)
{
    if (logger) {
        query(kind, askPermissions, std::make_shared<LoggedCallback>(kind, logger));
    }
}

void MediaAuthorization::query(MediaAuthorizationKind kind, bool askPermissions)
{
    query(kind, askPermissions, std::make_shared<NullCallback>());
}

bool MediaAuthorization::maybeAuthorized(MediaAuthorizationKind kind,
                                         const std::shared_ptr<Bricks::Logger>& logger)
{
    switch (request(kind, false, logger)) {
        case MediaAuthorizationStatus::NotDetermined:
            break;
        case MediaAuthorizationStatus::Granted:
        case MediaAuthorizationStatus::Restricted:
            return true;
        default:
            break;
    }
    return false;
}

std::string toString(MediaAuthorizationKind kind)
{
    switch (kind) {
        case MediaAuthorizationKind::Camera:
            return "camera";
        case MediaAuthorizationKind::Microphone:
            return "microphone";
        case MediaAuthorizationKind::ScreenCapturing:
            return "screen capturing";
        case MediaAuthorizationKind::WindowCapturing:
            return "window capturing";
        default:
            break;
    }
    return std::string();
}

MediaAuthorizationLevel mediaAuthorizationLevel()
{
    return g_mediaAuthorizationLevel;
}

void setMediaAuthorizationLevel(MediaAuthorizationLevel level)
{
    g_mediaAuthorizationLevel = level;
}

} // namespace LiveKitCpp

namespace {

MediaAuthorizationStatus SyncCallback::waitStatus()
{
    while (!_completed.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
    }
    return _status.load(std::memory_order_relaxed);
}

void SyncCallback::completed(MediaAuthorizationStatus status)
{
    _status = status;
    _completed = true;
}

LoggedCallback::LoggedCallback(MediaAuthorizationKind kind,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<MediaAuthorizationCallback>(logger)
    , _kind(kind)
{
}

void LoggedCallback::completed(MediaAuthorizationStatus status)
{
    logAuthorizationStatus(_kind, status, logger());
}

void logAuthorizationStatus(MediaAuthorizationKind kind,
                            MediaAuthorizationStatus status,
                            const std::shared_ptr<Bricks::Logger>& logger)
{
    if (logger) {
        switch (status) {
            case MediaAuthorizationStatus::NotDetermined:
                if (logger->canLogInfo()) {
                    logger->logInfo("User has made a choice regarding whether the application can access the "
                                    + toString(kind), g_category);
                }
                break;
            case MediaAuthorizationStatus::Granted:
                if (logger->canLogVerbose()) {
                    logger->logVerbose("The application is authorized to access the OS supporting a "
                                       + toString(kind), g_category);
                }
                break;
            case MediaAuthorizationStatus::Restricted:
                if (logger->canLogWarning()) {
                    logger->logWarning("The application is not authorized to access the OS for the "
                                       + toString(kind), g_category);
                }
                break;
            case MediaAuthorizationStatus::Denied:
                if (logger->canLogError()) {
                    logger->logError("User explicitly denied access to the OS supporting a "
                                     + toString(kind) + " for the application", g_category);
                }
                break;
            default: // ???
                break;
        }
    }
}

} // namespace
