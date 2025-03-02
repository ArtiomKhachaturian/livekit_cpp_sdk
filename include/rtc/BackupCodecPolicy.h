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
#pragma once // BackupCodecPolicy.h

namespace LiveKitCpp
{

// Policy for publisher to handle subscribers that are unable to support the primary codec of a track
enum BackupCodecPolicy
{
  // default behavior, regress to backup codec and all subscribers will receive the backup codec
  Regression = 0,
  // Encoding/Send The Primary And Backup Codec Simultaneously
  Simulcast = 1,
};

} // namespace LiveKitCpp
