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
#pragma once
#include "MediaDevice.h"
#include <api/peer_connection_interface.h>
#include <rtc_base/thread.h>
#include <memory>
#include <vector>

namespace rtc {
class SocketServer;
class SSLAdapterFactory;
class SSLAdapter;
} // namespace rtc

namespace webrtc {
class AudioEncoderFactory;
class AudioDecoderFactory;
class AudioMixer;
class FieldTrialsView;
class VideoEncoderFactory;
class VideoDecoderFactory;
class TaskQueueBase;
} // namespace webrtc

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AudioDeviceModuleListener;
class AudioDeviceProxyModule;
class WebRtcLogSink;

class PeerConnectionFactory : public webrtc::PeerConnectionFactoryInterface
{
public:
    ~PeerConnectionFactory() override;
    static webrtc::scoped_refptr<PeerConnectionFactory> Create(bool audioProcessing,
                                                               const std::shared_ptr<Bricks::Logger>& logger = {});
    const auto& eventsQueue() const noexcept { return _eventsQueue; }
    std::weak_ptr<rtc::Thread> signalingThread() const noexcept { return _signalingThread; }
    MediaDevice defaultRecordingAudioDevice() const;
    MediaDevice defaultPlayoutAudioDevice() const;
    bool setRecordingAudioDevice(const MediaDevice& device);
    MediaDevice recordingAudioDevice() const;
    bool setPlayoutAudioDevice(const MediaDevice& device);
    MediaDevice playoutAudioDevice() const;
    std::vector<MediaDevice> recordingAudioDevices() const;
    std::vector<MediaDevice> playoutAudioDevices() const;
    void addAdmListener(AudioDeviceModuleListener* listener);
    void removeAdmListener(AudioDeviceModuleListener* listener);
    // impl. of webrtc::PeerConnectionFactoryInterface
    void SetOptions(const Options& options) final;
    webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::PeerConnectionInterface>>
    CreatePeerConnectionOrError(const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
                                webrtc::PeerConnectionDependencies dependencies) final;
    webrtc::RtpCapabilities GetRtpSenderCapabilities(cricket::MediaType kind) const final;
    webrtc::RtpCapabilities GetRtpReceiverCapabilities(cricket::MediaType kind) const final;
    webrtc::scoped_refptr<webrtc::MediaStreamInterface> CreateLocalMediaStream(const std::string& streamId) final;
    webrtc::scoped_refptr<webrtc::AudioSourceInterface> CreateAudioSource(const cricket::AudioOptions& options) final;
    webrtc::scoped_refptr<webrtc::VideoTrackInterface> CreateVideoTrack(const std::string& label,
                                                                        webrtc::VideoTrackSourceInterface* source) final;
    webrtc::scoped_refptr<webrtc::VideoTrackInterface> CreateVideoTrack(webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
                                                                        absl::string_view label) final;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> CreateAudioTrack(const std::string& label,
                                                                        webrtc::AudioSourceInterface* source) final;
    bool StartAecDump(FILE* file, int64_t maxSizeBytes) final;
    void StopAecDump() final;
protected:
    PeerConnectionFactory(std::unique_ptr<WebRtcLogSink> webrtcLogSink,
                          std::shared_ptr<rtc::Thread> networkThread,
                          std::shared_ptr<rtc::Thread> workingThread,
                          std::shared_ptr<rtc::Thread> signalingThread,
                          webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> innerImpl,
                          webrtc::scoped_refptr<AudioDeviceProxyModule> adm);
private:
    const std::shared_ptr<webrtc::TaskQueueBase> _eventsQueue;
    const std::unique_ptr<WebRtcLogSink> _webrtcLogSink;
    const std::shared_ptr<rtc::Thread> _networkThread;
    const std::shared_ptr<rtc::Thread> _workingThread;
    const std::shared_ptr<rtc::Thread> _signalingThread;
    const webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _innerImpl;
    const webrtc::scoped_refptr<AudioDeviceProxyModule> _adm;
};

} // namespace LiveKitCpp
