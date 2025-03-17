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
#pragma once // RemoteAudioTrack.h
#include "AudioTrack.h"
#include "rtc/EncryptionType.h"
#include "rtc/BackupCodecPolicy.h"

namespace LiveKitCpp
{

class RemoteAudioTrack : public AudioTrack
{
public:
    virtual bool dtx() const = 0;
    virtual bool stereo() const = 0;
    virtual bool red() const = 0;
    virtual std::string mime() const = 0;
    virtual std::string stream() const = 0;
    virtual EncryptionType encryption() const = 0;
    virtual BackupCodecPolicy backupCodecPolicy() const = 0;
};

} // namespace LiveKitCpp
