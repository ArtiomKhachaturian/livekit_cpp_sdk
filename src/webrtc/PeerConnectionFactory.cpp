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
#include "LogsReceiver.h"
#ifdef __APPLE__
#include "MacAudioDeviceModule.h"
#endif
#include "VideoDecoderFactory.h"
#include "VideoEncoderFactory.h"
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

inline std::unique_ptr<rtc::Thread> CreateRunningThread(bool withSocketServer,
                                                        const absl::string_view& threadName,
                                                        const std::shared_ptr<LiveKitCpp::LogsReceiver>& logger = {})
{
    if (auto thread = withSocketServer ? rtc::Thread::CreateWithSocketServer() : rtc::Thread::Create()) {
        thread->SetName(threadName, thread.get());
        thread->AllowInvokesToThread(thread.get());
        if (thread->Start()) {
            return thread;
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

PeerConnectionFactory::PeerConnectionFactory(std::unique_ptr<WebRtcLogSink> webrtcLogSink,
                                             std::unique_ptr<rtc::Thread> networkThread,
                                             std::unique_ptr<rtc::Thread> workingThread,
                                             std::unique_ptr<rtc::Thread> signalingThread,
                                             webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> innerImpl)
    : _webrtcLogSink(std::move(webrtcLogSink))
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
}

PeerConnectionFactory::~PeerConnectionFactory()
{
}

const std::shared_ptr<LogsReceiver>& PeerConnectionFactory::logger() const noexcept
{
    return _webrtcLogSink->logger();
}

webrtc::scoped_refptr<PeerConnectionFactory> PeerConnectionFactory::Create(bool audioProcessing,
                                                                           bool customAdm,
                                                                           const std::shared_ptr<LogsReceiver>& logger)
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
    dependencies.video_decoder_factory = webrtc::CreateBuiltinVideoDecoderFactory();
    dependencies.video_encoder_factory = webrtc::CreateBuiltinVideoEncoderFactory();
    dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
    dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    if (audioProcessing) {
        dependencies.audio_processing_builder = std::make_unique<webrtc::BuiltinAudioProcessingBuilder>();
    }
#ifdef __APPLE__
    if (customAdm) {
        workingThread->BlockingCall([&dependencies]() {
            dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
            dependencies.adm = rtc::make_ref_counted<MacAudioDeviceModule>(dependencies.task_queue_factory.get());
        });
    }
#endif
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
                                                               std::move(pcf));
    }
    if (logger) {
        logger->logError("Failed to create modular peer connection factory", g_pcfInit);
    }
    return {};
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

} // namespace LiveKitCpp

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
