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
#pragma once // 

namespace LiveKitCpp
{

// Based on the spec in
// https://w3c.github.io/webrtc-pc/#idl-def-rtcdegradationpreference.
// These options are enforced on a best-effort basis. For instance, all of
// these options may suffer some frame drops in order to avoid queuing.
// TODO(sprang): Look into possibility of more strictly enforcing the
// maintain-framerate option.
// TODO(deadbeef): Default to "balanced", as the spec indicates?
enum class DegradationPreference 
{
    // System choice
    Default,
    // Don't take any actions based on over-utilization signals. Not part of the
    // web API.
    Disabled,
    // On over-use, request lower resolution, possibly causing down-scaling.
    MaintainFramerate,
    // On over-use, request lower frame rate, possibly causing frame drops.
    MaintainResolution,
    // Try to strike a "pleasing" balance between frame rate or resolution.
    Balanced,
};

} // namespace LiveKitCpp
