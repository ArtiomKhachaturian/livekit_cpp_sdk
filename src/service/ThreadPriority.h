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
#pragma once // ThreadPriority.h
#ifdef _WIN32
#include <Windows.h>
#endif

namespace LiveKitCpp
{

enum class ThreadPriority : int
{
#ifdef _WIN32
    Auto     = THREAD_BASE_PRIORITY_MIN,
    Low      = THREAD_PRIORITY_BELOW_NORMAL,
    Normal   = THREAD_PRIORITY_NORMAL,
    High     = THREAD_PRIORITY_ABOVE_NORMAL,
    Highest  = THREAD_PRIORITY_HIGHEST,
    Realtime = THREAD_PRIORITY_TIME_CRITICAL
#else
    Auto     = 0,
    Low      = 1,
    Normal   = 2,
    High     = 3,
    Highest  = 4,
    Realtime = 5
#endif
};

const char* ToString(ThreadPriority priority);

} // namespace LiveKitCpp
