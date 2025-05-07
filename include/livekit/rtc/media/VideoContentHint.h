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
#pragma once // VideoContentHint.h

namespace LiveKitCpp
{

// Video track content hint, used to override the source is_screencast
// property.
// See https://crbug.com/653531 and https://w3c.github.io/mst-content-hint.
enum class VideoContentHint
{
    None,
    Motion,   // only for camera sources
    Detailed, // only for screencast sources
    Text      // only for screencast sources
};
	
} // namespace LiveKitCpp
