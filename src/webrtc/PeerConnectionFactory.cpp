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
#include "PeerConnectionFactory.h"
#include "WebRtcLogSink.h"
#include "Logger.h"
#include "Utils.h"
#include "ThreadUtils.h"
#include "AdmProxy.h"
#include "AdmProxyFacade.h"
#include "VideoDecoderFactory.h"
#include "VideoEncoderFactory.h"
#include "media/MicrophoneOptions.h"
#include <api/audio/builtin_audio_processing_builder.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/audio/audio_processing.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/enable_media.h>
#include <media/engine/webrtc_media_engine.h>
#include <modules/audio_mixer/audio_mixer_impl.h>

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace {

static const std::string_view g_pcfInit("PeerConnectionFactory_Init");

inline std::shared_ptr<rtc::Thread> CreateRunningThread(bool withSocketServer,
                                                        const absl::string_view& threadName,
                                                        const std::shared_ptr<Bricks::Logger>& logger = {})
{
    if (auto thread = withSocketServer ? rtc::Thread::CreateWithSocketServer() : rtc::Thread::Create()) {
        thread->SetName(threadName, thread.get());
        thread->AllowInvokesToThread(thread.get());
        if (thread->Start()) {
            return std::shared_ptr<rtc::Thread>(thread.release());
        }
        if (logger) {
            logger->logError("Failed to start of '" + std::string(threadName)
                             + "' thread", g_pcfInit);
        }
    }
    else if (logger) {
        logger->logError("Failed to create of '" + std::string(threadName)
                         + "' thread", g_pcfInit);
    }
    return nullptr;
}

inline cricket::AudioOptions toCricketOptions(const LiveKitCpp::MicrophoneOptions& options)
{
    cricket::AudioOptions audioOptions;
    audioOptions.echo_cancellation = options._echoCancellation;
    audioOptions.auto_gain_control = options._autoGainControl;
    audioOptions.noise_suppression = options._noiseSuppression;
    audioOptions.highpass_filter = options._highpassFilter;
    audioOptions.stereo_swapping = options._stereoSwapping;
    return audioOptions;
}

}

namespace webrtc {

std::unique_ptr<VideoDecoderFactory> CreateBuiltinVideoDecoderFactory() {
    return std::make_unique<LiveKitCpp::VideoDecoderFactory>();
}

std::unique_ptr<VideoEncoderFactory> CreateBuiltinVideoEncoderFactory() {
    return std::make_unique<LiveKitCpp::VideoEncoderFactory>();
}

}

namespace LiveKitCpp
{

class PeerConnectionFactory::AdmFacade : public AdmProxyFacade
{
public:
    AdmFacade(webrtc::scoped_refptr<AdmProxy> admProxy, cricket::AudioOptions options);
    ~AdmFacade() final { close(); }
    void close() { _admProxy->close(); }
    auto playoutDevices() const { return _admProxy->playoutDevices(); }
    auto recordingDevices() const { return _admProxy->recordingDevices(); }
    auto defaultRecordingDevice() const { return _admProxy->defaultRecordingDevice(); }
    auto defaultPlayoutDevice() const { return _admProxy->defaultPlayoutDevice(); }
    bool setRecordingDevice(const MediaDeviceInfo& info);
    bool setPlayoutDevice(const MediaDeviceInfo& info);
    bool setMicrophoneVolume(double volume);
    bool setSpeakerVolume(double volume);
    bool setRecordingMute(bool mute);
    bool setPlayoutMute(bool mute);
    // impl. of AdmProxyFacade
    void registerRecordingSink(webrtc::AudioTrackSinkInterface* sink, bool reg) final;
    void registerRecordingListener(AdmProxyListener* l, bool reg) final;
    void registerPlayoutListener(AdmProxyListener* l, bool reg) final;
    cricket::AudioOptions options() const final { return _options; }
    const AdmProxyState& recordingState() const final { return _admProxy->recordingState(); }
    const AdmProxyState& playoutState() const final { return _admProxy->playoutState(); }
private:
    const webrtc::scoped_refptr<AdmProxy> _admProxy;
    const cricket::AudioOptions _options;
};

PeerConnectionFactory::PeerConnectionFactory(std::unique_ptr<WebRtcLogSink> webrtcLogSink,
                                             std::shared_ptr<rtc::Thread> networkThread,
                                             std::shared_ptr<rtc::Thread> workingThread,
                                             std::shared_ptr<rtc::Thread> signalingThread,
                                             webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> innerImpl,
                                             webrtc::scoped_refptr<AdmProxy> admProxy,
                                             const MicrophoneOptions& microphoneOptions)
    : _eventsQueue(createTaskQueueS("events_queue"))
    , _webrtcLogSink(std::move(webrtcLogSink))
    , _networkThread(std::move(networkThread))
    , _workingThread(std::move(workingThread))
    , _signalingThread(std::move(signalingThread))
    , _innerImpl(std::move(innerImpl))
{
    // check GCM suites for DTLS-SRTP are enabled
    webrtc::PeerConnectionFactoryInterface::Options peerConnectionFactoryOptions;
    peerConnectionFactoryOptions.crypto_options.srtp.enable_gcm_crypto_suites = true;
    _innerImpl->SetOptions(std::move(peerConnectionFactoryOptions));
    _networkThread->AllowInvokesToThread(_networkThread.get());
    _workingThread->AllowInvokesToThread(_workingThread.get());
    _signalingThread->AllowInvokesToThread(_signalingThread.get());
    if (admProxy) {
        auto options = toCricketOptions(microphoneOptions);
        _admProxy.reset(new AdmFacade(std::move(admProxy), std::move(options)));
    }
}

PeerConnectionFactory::~PeerConnectionFactory()
{
    if (_admProxy) {
        _admProxy->close();
    }
}

webrtc::scoped_refptr<PeerConnectionFactory> PeerConnectionFactory::
    create(bool audioProcessing, const MicrophoneOptions& microphoneOptions,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    //create threads for peer connection factory
    //See also https://webrtc.org/native-code/native-apis/#threading-model
    auto networkThread = CreateRunningThread(true, "network_thread", logger);
    if (!networkThread) {
        return {};
    }
    auto workingThread = CreateRunningThread(false, "working_thread", logger);
    if (!workingThread) {
        return {};
    }
    auto signalingThread = CreateRunningThread(false, "signaling_thread", logger);
    if (!signalingThread) {
        return {};
    }
    // setup dependencies
    webrtc::PeerConnectionFactoryDependencies dependencies;
    // initialize
    dependencies.network_thread = networkThread.get();
    dependencies.worker_thread = workingThread.get();
    dependencies.signaling_thread = signalingThread.get();
    dependencies.task_queue_factory = createTaskQueueFactory();
    dependencies.video_decoder_factory = webrtc::CreateBuiltinVideoDecoderFactory();
    dependencies.video_encoder_factory = webrtc::CreateBuiltinVideoEncoderFactory();
    dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
    dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    auto admProxy = AdmProxy::create(workingThread, signalingThread,
                                     dependencies.task_queue_factory.get());
    if (audioProcessing) {
        dependencies.audio_processing_builder = std::make_unique<webrtc::BuiltinAudioProcessingBuilder>();
    }
    dependencies.adm = admProxy;
    dependencies.event_log_factory = nullptr; // should be NULL or customized and adapted to our log system
    webrtc::EnableMedia(dependencies);
    //create inner factory
    if (auto pcf = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies))) {
        std::unique_ptr<WebRtcLogSink> webrtcLogSink;
        if (logger) {
            webrtcLogSink = std::make_unique<WebRtcLogSink>(logger);
        }
        return webrtc::make_ref_counted<PeerConnectionFactory>(std::move(webrtcLogSink),
                                                               std::move(networkThread),
                                                               std::move(workingThread),
                                                               std::move(signalingThread),
                                                               std::move(pcf),
                                                               std::move(admProxy),
                                                               microphoneOptions);
    }
    return {};
}
                              
std::weak_ptr<AdmProxyFacade> PeerConnectionFactory::admProxy() const
{
    return _admProxy;
}

MediaDeviceInfo PeerConnectionFactory::defaultAudioRecordingDevice() const
{
    if (_admProxy) {
        return _admProxy->defaultRecordingDevice();
    }
    return {};
}

MediaDeviceInfo PeerConnectionFactory::defaultAudioPlayoutDevice() const
{
    if (_admProxy) {
        return _admProxy->defaultPlayoutDevice();
    }
    return {};
}

bool PeerConnectionFactory::setAudioRecordingDevice(const MediaDeviceInfo& info)
{
    return _admProxy && _admProxy->setRecordingDevice(info);
}

MediaDeviceInfo PeerConnectionFactory::recordingAudioDevice() const
{
    if (_admProxy) {
        return _admProxy->recordingState().currentDevice();
    }
    return {};
}

bool PeerConnectionFactory::setAudioPlayoutDevice(const MediaDeviceInfo& info)
{
    return _admProxy && _admProxy->setPlayoutDevice(info);
}

MediaDeviceInfo PeerConnectionFactory::playoutAudioDevice() const
{
    if (_admProxy) {
        return _admProxy->playoutState().currentDevice();
    }
    return {};
}

std::vector<MediaDeviceInfo> PeerConnectionFactory::recordingAudioDevices() const
{
    if (_admProxy) {
        return _admProxy->recordingDevices();
    }
    return {};
}

std::vector<MediaDeviceInfo> PeerConnectionFactory::playoutAudioDevices() const
{
    if (_admProxy) {
        return _admProxy->playoutDevices();
    }
    return {};
}

void PeerConnectionFactory::setMicrophoneVolume(double volume)
{
    postAdmTask(&AdmFacade::setMicrophoneVolume, volume);
}

void PeerConnectionFactory::setSpeakerVolume(double volume)
{
    postAdmTask(&AdmFacade::setSpeakerVolume, volume);
}

void PeerConnectionFactory::setRecordingMute(bool mute)
{
    postAdmTask(&AdmFacade::setRecordingMute, mute);
}

void PeerConnectionFactory::setPlayoutMute(bool mute)
{
    postAdmTask(&AdmFacade::setPlayoutMute, mute);
}

void PeerConnectionFactory::registerAdmRecordingListener(AdmProxyListener* l, bool reg)
{
    if (_admProxy) {
        _admProxy->registerRecordingListener(l, reg);
    }
}

void PeerConnectionFactory::registerAdmPlayoutListener(AdmProxyListener* l, bool reg)
{
    if (_admProxy) {
        _admProxy->registerPlayoutListener(l, reg);
    }
}

void PeerConnectionFactory::SetOptions(const Options& options)
{
    _innerImpl->SetOptions(options);
}

webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::PeerConnectionInterface>> PeerConnectionFactory::
    CreatePeerConnectionOrError(const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
                                webrtc::PeerConnectionDependencies dependencies)
{
    return _innerImpl->CreatePeerConnectionOrError(configuration, std::move(dependencies));
}

webrtc::RtpCapabilities PeerConnectionFactory::GetRtpSenderCapabilities(cricket::MediaType kind) const
{
    return _innerImpl->GetRtpSenderCapabilities(kind);
}

webrtc::RtpCapabilities PeerConnectionFactory::GetRtpReceiverCapabilities(cricket::MediaType kind) const
{
    return _innerImpl->GetRtpReceiverCapabilities(kind);
}

webrtc::scoped_refptr<webrtc::MediaStreamInterface> PeerConnectionFactory::
    CreateLocalMediaStream(const std::string& streamId)
{
    return _innerImpl->CreateLocalMediaStream(streamId);
}

webrtc::scoped_refptr<webrtc::AudioSourceInterface> PeerConnectionFactory::
    CreateAudioSource(const cricket::AudioOptions& options)
{
    return _innerImpl->CreateAudioSource(options);
}

webrtc::scoped_refptr<webrtc::VideoTrackInterface> PeerConnectionFactory::
    CreateVideoTrack(const std::string& label, webrtc::VideoTrackSourceInterface* source)
{
    return _innerImpl->CreateVideoTrack(label, source);
}

webrtc::scoped_refptr<webrtc::VideoTrackInterface> PeerConnectionFactory::
    CreateVideoTrack(webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
                     absl::string_view label)
{
    return _innerImpl->CreateVideoTrack(std::move(source), label);
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> PeerConnectionFactory::
    CreateAudioTrack(const std::string& label, webrtc::AudioSourceInterface* source)
{
    return _innerImpl->CreateAudioTrack(label, source);
}

bool PeerConnectionFactory::StartAecDump(FILE* file, int64_t maxSizeBytes)
{
    return _innerImpl->StartAecDump(file, maxSizeBytes);
}

void PeerConnectionFactory::StopAecDump()
{
    _innerImpl->StopAecDump();
}

template <class Method, typename... Args>
void PeerConnectionFactory::postAdmTask(Method method, Args&&... args) const
{
    if (_admProxy) {
        postOrInvokeS(_workingThread, _admProxy, true,
                      std::move(method),
                      std::forward<Args>(args)...);
    }
}

PeerConnectionFactory::AdmFacade::AdmFacade(webrtc::scoped_refptr<AdmProxy> admProxy,
                                            cricket::AudioOptions options)
    : _admProxy(std::move(admProxy))
    , _options(std::move(options))
{
}

bool PeerConnectionFactory::AdmFacade::setRecordingDevice(const MediaDeviceInfo& info)
{
    return _admProxy->setRecordingDevice(info);
}

bool PeerConnectionFactory::AdmFacade::setPlayoutDevice(const MediaDeviceInfo& info)
{
    return _admProxy->setPlayoutDevice(info);
}

bool PeerConnectionFactory::AdmFacade::setMicrophoneVolume(double volume)
{
    return _admProxy->setMicrophoneVolume(volume);
}

bool PeerConnectionFactory::AdmFacade::setSpeakerVolume(double volume)
{
    return _admProxy->setSpeakerVolume(volume);
}

bool PeerConnectionFactory::AdmFacade::setRecordingMute(bool mute)
{
    return 0 == _admProxy->SetMicrophoneMute(mute);
}

bool PeerConnectionFactory::AdmFacade::setPlayoutMute(bool mute)
{
    return 0 == _admProxy->SetSpeakerMute(mute);
}

void PeerConnectionFactory::AdmFacade::registerRecordingSink(webrtc::AudioTrackSinkInterface* sink,
                                                             bool reg)
{
    _admProxy->registerRecordingSink(sink, reg);
}

void PeerConnectionFactory::AdmFacade::registerRecordingListener(AdmProxyListener* l,
                                                                 bool reg)
{
    _admProxy->registerRecordingListener(l, reg);
}

void PeerConnectionFactory::AdmFacade::registerPlayoutListener(AdmProxyListener* l,
                                                               bool reg)
{
    _admProxy->registerPlayoutListener(l, reg);
}

} // namespace LiveKitCpp

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
