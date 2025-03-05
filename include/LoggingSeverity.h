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
#pragma once // LoggingSeverity.h

namespace LiveKitCpp
{

// The meanings of the levels are:
//  Verbose: This level is for data which we do not want to appear in the
//   normal debug log, but should appear in diagnostic logs.
//  Info: Chatty level used in debugging for all sorts of things, the default
//   in debug builds.
//  Warning: Something that may warrant investigation.
//  Error: Something that should not have occurred.
enum class LoggingSeverity
{
    Verbose,
    Info,
    Warning,
    Error,
};

} // namespace LiveKitCpp
