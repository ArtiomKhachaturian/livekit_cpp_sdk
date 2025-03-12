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
#pragma once // Track.h
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <string>

namespace LiveKitCpp
{

class Track
{
public:
    virtual ~Track() = default;
    virtual bool remote() const noexcept = 0;
    // type
    virtual cricket::MediaType mediaType() const = 0;
    // server track ID
    virtual std::string sid() const = 0;
    // mute/unmute state
    virtual void mute(bool mute) = 0;
    virtual bool muted() const = 0;
};

} // namespace LiveKitCpp
