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
#pragma once // MediaAuthorizationStatus.h

namespace LiveKitCpp
{

enum class MediaAuthorizationStatus
{
    // explicit user permission is required for media capture, but the user has not yet granted or denied such permission
    NotDetermined,
    // the client is authorized to access the system supporting a media type
    Granted,
    // the user explicitly denied access to the system supporting a media type for the client
    Denied,
    // the client is not authorized to access the system for the media type,
    // the user cannot change the client's status, possibly due to active restrictions such as parental controls being in place
    Restricted
};

} // namespace LiveKitCpp
