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
#include "AudioDevicesEnumerator.h"
#include <modules/audio_device/include/audio_device.h> //AudioDeviceModule

namespace LiveKitCpp
{

AudioDevicesEnumerator::AudioDevicesEnumerator(bool recording,
                                               const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
    : _recording(recording)
    , _adm(adm)
{
}

AudioDevicesEnumerator::~AudioDevicesEnumerator()
{
}

void AudioDevicesEnumerator::enumerate(const Acceptor& acceptor) const
{
    if (_adm) {
        const int16_t count = _recording ? _adm->RecordingDevices() : _adm->PlayoutDevices();
        if (count > 0) {
            static thread_local char name[webrtc::kAdmMaxDeviceNameSize] = {};
            static thread_local char guid[webrtc::kAdmMaxDeviceNameSize] = {};
            for (uint16_t i = 0U, ndx = 0; i < count; ++i) {
                bool ok = false;
                if (_recording) {
                    ok = 0 == _adm->RecordingDeviceName(i, name, guid);
                }
                else {
                    ok = 0 == _adm->PlayoutDeviceName(i, name, guid);
                }
                if (ok && !acceptor(ndx++, name, guid)) {
                    break;
                }
            }
        }
    }
}

void AudioDevicesEnumerator::enumerate(const Consumer& consumer) const
{
    enumerate([&consumer](uint16_t, std::string_view name, std::string_view guid) {
        consumer(name, guid);
        return true;
    });
}

} // namespace LiveKitCpp
