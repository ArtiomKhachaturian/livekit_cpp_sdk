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
#pragma once // MediaAuthorizationLevel.h

namespace LiveKitCpp
{

/** This enum represents the policy for request of the user’s media permissions on the SDK side,
 *  if needed, for recording a specified media type, such as camera, microphone and screen/window sharing objects.
 *  Just for info - some system media functions maybe be a reason of prompting messageboxes appearing
 *  (independently of current authorization level), because they send mplicit requests to the OS to determine
 *  media permissions for the application process.
 *  [CheckOnly] (level) by default in SDK, for avoiding of conflicts with customer's processes/applications.
 */
enum class MediaAuthorizationLevel
{
    /** Owner application/process is responsible  for verification & request for media permissions.  */
    Disabled,
    /** Allows  to checks whether the current process already has permission for media recording, without prompt the user for recording permission. */
    CheckOnly,
    /** Allows to request the user’s permission if absent, for recording a specified media type. */
    RequestIfNeeded
};

} // namespace LiveKitCpp
