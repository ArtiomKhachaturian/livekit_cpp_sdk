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
#include "Utils.h"

namespace LiveKitCpp
{

void MediaAuthorization::requestFromSystem(MediaAuthorizationKind kind, bool /*askPermissions*/,
                                           const std::shared_ptr<MediaAuthorizationCallback>& callback)
{
    if (callback) {
        MediaAuthorizationStatus status = MediaAuthorizationStatus::Granted;
        const char* registrySubKey = nullptr;
        switch (kind) {
        case MediaAuthorizationKind::Camera:
            registrySubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam";
            break;
        case MediaAuthorizationKind::Microphone:
            registrySubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone";
            break;
        default:
            break;
        }
        if (registrySubKey) {
            const auto valueData = queryRegistryValue(HKEY_CURRENT_USER, registrySubKey, "Value");
            if (!valueData.empty()) {
                const char* permission = reinterpret_cast<const char*>(valueData.data());
                if (0 == ::stricmp(permission, "deny")) {
                    status = MediaAuthorizationStatus::Denied;
                } else if (0 != ::stricmp(permission, "allow")) {
                    status = MediaAuthorizationStatus::NotDetermined;
                }
            }
        }
        callback->completed(status);
    }
}

} // namespace LiveKitCpp
