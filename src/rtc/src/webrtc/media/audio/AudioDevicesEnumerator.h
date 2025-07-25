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
#pragma once // AudioDevicesEnumerator.h
#include <api/scoped_refptr.h>
#include <functional>
#include <string_view>
#include <vector>

namespace webrtc {
class AudioDeviceModule;
}

namespace LiveKitCpp
{

class AudioDevicesEnumerator
{
public:
    // return false for stop enumeration
    // 1st arg - device index (zero-based)
    // 2nd - name
    // 3rd - guid
    using Acceptor = std::function<bool(uint16_t, std::string_view, std::string_view)>;
    using Consumer = std::function<void(std::string_view, std::string_view)>;
public:
    AudioDevicesEnumerator(bool recording,
                           const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    ~AudioDevicesEnumerator();
    void enumerate(const Acceptor& acceptor) const;
    void enumerate(const Consumer& consumer) const;
private:
    const bool _recording;
    const webrtc::scoped_refptr<webrtc::AudioDeviceModule> _adm;
};

} // namespace LiveKitCpp
