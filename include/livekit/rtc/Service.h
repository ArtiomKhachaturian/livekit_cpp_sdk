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
#pragma once // Service.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/Options.h"
#include "livekit/rtc/Session.h"
#include "livekit/rtc/ServiceState.h"
#include "livekit/rtc/ServiceInitInfo.h"
#include "livekit/rtc/media/AudioDevice.h"
#include "livekit/rtc/media/LocalVideoDevice.h"
#include "livekit/rtc/media/AudioRecordingOptions.h"
#include "livekit/rtc/media/MediaAuthorizationLevel.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <memory>
#include <vector>
#include <stdio.h>

namespace Websocket {
class Factory;
}

namespace LiveKitCpp
{

class AudioFramesWriter;
class ServiceListener;
enum class NetworkType;

class LIVEKIT_RTC_API Service
{
    class Impl;
public:
    Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
            ServiceInitInfo initInfo = {});
    ~Service();
    ServiceState state() const;
    std::unique_ptr<Session> createSession(Options options = {}) const;
    // local media
    std::unique_ptr<AudioDevice> createMicrophone(const AudioRecordingOptions& options = {}) const;
    std::unique_ptr<LocalVideoDevice> createCamera(MediaDeviceInfo info = {}, VideoOptions options = {}) const;
    std::unique_ptr<LocalVideoDevice> createSharing(bool previewMode,
                                                    MediaDeviceInfo info = {}, VideoOptions options = {}) const;
    // global media
    MediaDeviceInfo defaultAudioRecordingDevice() const;
    MediaDeviceInfo defaultAudioPlayoutDevice() const;
    // device for input from micrpohone
    bool setAudioRecordingDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo recordingAudioDevice() const;
    // normalized volume in range [0...1]
    double recordingAudioVolume() const;
    void setRecordingAudioVolume(double volume);
    // device for output from speakers
    bool setAudioPlayoutDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo playoutAudioDevice() const;
    // normalized volume in range [0...1]
    double playoutAudioVolume() const;
    void setPlayoutAudioVolume(double volume);
    // mute/unmute for speakers & microphone
    void setAudioRecordingEnabled(bool enabled);
    bool audioRecordingEnabled() const;
    void setAudioPlayoutEnabled(bool enabled);
    bool audioPlayoutEnabled() const;
    // enumeration
    std::vector<std::string> videoEncoderFormats() const;
    std::vector<std::string> videoDecoderFormats() const;
    std::vector<std::string> audioEncoderFormats() const;
    std::vector<std::string> audioDecoderFormats() const;
    std::vector<MediaDeviceInfo> screens() const;
    std::vector<MediaDeviceInfo> windows() const;
    std::vector<MediaDeviceInfo> recordingAudioDevices() const;
    std::vector<MediaDeviceInfo> playoutAudioDevices() const;
    std::vector<MediaDeviceInfo> cameraDevices() const;
    std::vector<VideoOptions> cameraOptions(const MediaDeviceInfo& info) const;
    bool displayCameraSettingsDialogBox(const MediaDeviceInfo& dev,
                                        std::string_view dialogTitleUTF8,
                                        void* parentWindow,
                                        uint32_t positionX, uint32_t positionY) const;
    void enableAudioRecordingProcessing(bool enable);
    void enableAudioPlayoutProcessing(bool enable);
    bool audioRecordingProcessingEnabled() const;
    bool audioPlayoutProcessingEnabled() const;
    // AEC
    // Starts AEC dump using existing file. Takes ownership of `file` and passes
    // it on to VoiceEngine (via other objects) immediately, which will take
    // the ownerhip. If the operation fails, the file will be closed.
    // A maximum file size in bytes can be specified. When the file size limit is
    // reached, logging is stopped automatically. If maxSizeBytes is set to a
    // value <= 0, no limit will be used, and logging will continue until the
    // stopAecDump function is called.
    bool startAecDump(FILE* file, int64_t maxSizeBytes = 0);
    void stopAecDump();
    void setRecordingFramesWriter(AudioFramesWriter* writer = nullptr);
    void setPlayoutFramesWriter(AudioFramesWriter* writer = nullptr);
    // listeners
    void addListener(ServiceListener* listener);
    void removeListener(ServiceListener* listener);
    // media auth
    static MediaAuthorizationLevel mediaAuthorizationLevel();
    static void setMediaAuthorizationLevel(MediaAuthorizationLevel level);
    // network
    static NetworkType activeNetworkType();
    // default camera options
    static VideoOptions defaultCameraOptions();
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
