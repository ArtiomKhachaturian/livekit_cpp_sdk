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
#pragma once // MediaAuthorization.h
#include "MediaAuthorizationKind.h"
#include "MediaAuthorizationStatus.h"
#include "MediaAuthorizationLevel.h"
#include <memory>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class MediaAuthorizationCallback;

class MediaAuthorization
{
public:
    // sync version
    [[nodiscard]] static MediaAuthorizationStatus request(MediaAuthorizationKind kind,
                                                          bool askPermissions,
                                                          const std::shared_ptr<Bricks::Logger>& logger = {});
    // sync/async version, depends on platform impl.
    static void query(MediaAuthorizationKind kind, bool askPermissions,
                        const std::shared_ptr<MediaAuthorizationCallback>& callback);
    static void query(MediaAuthorizationKind kind, bool askPermissions,
                      const std::shared_ptr<Bricks::Logger>& logger);
    // helper function, suitable for most cases
    static bool maybeAuthorized(MediaAuthorizationKind kind,
                                const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    // platform impl.
    static void requestFromSystem(MediaAuthorizationKind kind, bool askPermissions,
                                  const std::shared_ptr<MediaAuthorizationCallback>& callback);
};

MediaAuthorizationLevel mediaAuthorizationLevel();
void setMediaAuthorizationLevel(MediaAuthorizationLevel level);

} // namespace LiveKitCpp
