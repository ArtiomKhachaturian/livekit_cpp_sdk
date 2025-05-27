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
#include "AudioProcessingWrapper.h"

namespace LiveKitCpp
{

AudioProcessingWrapper::AudioProcessingWrapper(webrtc::scoped_refptr<webrtc::AudioProcessing> impl)
    : _impl(std::move(impl))
{
}

int AudioProcessingWrapper::Initialize()
{
    return _impl ? _impl->Initialize() : kNullPointerError;
}

int AudioProcessingWrapper::Initialize(const webrtc::ProcessingConfig& processingConfig)
{
    return _impl ? _impl->Initialize(processingConfig) : kNullPointerError;
}

void AudioProcessingWrapper::ApplyConfig(const webrtc::AudioProcessing::Config& config)
{
    if (_impl) {
        _impl->ApplyConfig(config);
    }
}

bool AudioProcessingWrapper::CreateAndAttachAecDump(absl::string_view fileName,
                                                    int64_t maxLogSizeBytes,
                                                    absl::Nonnull<webrtc::TaskQueueBase*> workerQueue)
{
    return _impl && _impl->CreateAndAttachAecDump(std::move(fileName), maxLogSizeBytes, std::move(workerQueue));
}

bool AudioProcessingWrapper::CreateAndAttachAecDump(FILE* handle, int64_t maxLogSizeBytes,
                                                    absl::Nonnull<webrtc::TaskQueueBase*> workerQueue)
{
    return _impl && _impl->CreateAndAttachAecDump(handle, maxLogSizeBytes, std::move(workerQueue));
}

void AudioProcessingWrapper::AttachAecDump(std::unique_ptr<webrtc::AecDump> aecDump)
{
    if (_impl) {
        _impl->AttachAecDump(std::move(aecDump));
    }
}

void AudioProcessingWrapper::DetachAecDump()
{
    if (_impl) {
        _impl->DetachAecDump();
    }
}

void AudioProcessingWrapper::SetRuntimeSetting(RuntimeSetting setting)
{
    if (_impl) {
        _impl->SetRuntimeSetting(std::move(setting));
    }
}

bool AudioProcessingWrapper::PostRuntimeSetting(RuntimeSetting setting)
{
    return _impl && _impl->PostRuntimeSetting(std::move(setting));
}

int AudioProcessingWrapper::ProcessStream(const int16_t* const src,
                                          const webrtc::StreamConfig& inputConfig,
                                          const webrtc::StreamConfig& outputConfig,
                                          int16_t* const dest)
{
    int result = kNullPointerError;
    if (_impl) {
        return _impl->ProcessStream(src, inputConfig, outputConfig, dest);
    }
    return kNullPointerError;
}

int AudioProcessingWrapper::ProcessStream(const float* const* src,
                                          const webrtc::StreamConfig& inputConfig,
                                          const webrtc::StreamConfig& outputConfig,
                                          float* const* dest)
{
    int result = kNullPointerError;
    if (_impl) {
        return _impl->ProcessStream(src, inputConfig, outputConfig, dest);
    }
    return kNullPointerError;
}

bool AudioProcessingWrapper::GetLinearAecOutput(webrtc::ArrayView<std::array<float, 160>> linearOutput) const
{
    return _impl && _impl->GetLinearAecOutput(std::move(linearOutput));
}

void AudioProcessingWrapper::set_output_will_be_muted(bool muted)
{
    if (_impl) {
        _impl->set_output_will_be_muted(muted);
    }
}

int AudioProcessingWrapper::set_stream_delay_ms(int delay)
{
    if (_impl) {
        return _impl->set_stream_delay_ms(delay);
    }
    return kNullPointerError;
}

void AudioProcessingWrapper::set_stream_key_pressed(bool keyPressed)
{
    if (_impl) {
        _impl->set_stream_key_pressed(keyPressed);
    }
}

void AudioProcessingWrapper::set_stream_analog_level(int level)
{
    if (_impl) {
        _impl->set_stream_analog_level(level);
    }
}

int AudioProcessingWrapper::recommended_stream_analog_level() const
{
    if (_impl) {
        _impl->recommended_stream_analog_level();
    }
    return 255;
}

int AudioProcessingWrapper::ProcessReverseStream(const int16_t* const src,
                                                 const webrtc::StreamConfig& inputConfig,
                                                 const webrtc::StreamConfig& outputConfig,
                                                 int16_t* const dest)
{
    if (_impl) {
        return _impl->ProcessReverseStream(src, inputConfig, outputConfig, dest);
    }
    return kNullPointerError;
}

int AudioProcessingWrapper::AnalyzeReverseStream(const float* const* data,
                                                 const webrtc::StreamConfig& reverseConfig)
{
    if (_impl) {
        return _impl->AnalyzeReverseStream(data, reverseConfig);
    }
    return kNullPointerError;
}

int AudioProcessingWrapper::ProcessReverseStream(const float* const* src,
                                                 const webrtc::StreamConfig& inputConfig,
                                                 const webrtc::StreamConfig& outputConfig,
                                                 float* const* dest)
{
    if (_impl) {
        return _impl->ProcessReverseStream(src, inputConfig, outputConfig, dest);
    }
    return kNullPointerError;
}

int AudioProcessingWrapper::proc_sample_rate_hz() const
{
    return _impl ? _impl->proc_sample_rate_hz() : 0;
}

int AudioProcessingWrapper::proc_split_sample_rate_hz() const
{
    return _impl ? _impl->proc_split_sample_rate_hz() : 0;
}

size_t AudioProcessingWrapper::num_input_channels() const
{
    return _impl ? _impl->num_input_channels() : 0U;
}

size_t AudioProcessingWrapper::num_proc_channels() const
{
    return _impl ? _impl->num_proc_channels() : 0U;
}

size_t AudioProcessingWrapper::num_output_channels() const
{
    return _impl ? _impl->num_output_channels() : 0U;
}

size_t AudioProcessingWrapper::num_reverse_channels() const
{
    return _impl ? _impl->num_reverse_channels() : 0U;
}

int AudioProcessingWrapper::stream_delay_ms() const
{
    return _impl ? _impl->stream_delay_ms() : 0;
}

webrtc::AudioProcessingStats AudioProcessingWrapper::GetStatistics(bool hasRemoteTracks)
{
    if (_impl) {
        return _impl->GetStatistics(hasRemoteTracks);
    }
    return {};
}

webrtc::AudioProcessingStats AudioProcessingWrapper::GetStatistics()
{
    if (_impl) {
        return _impl->GetStatistics();
    }
    return {};
}

webrtc::AudioProcessing::Config AudioProcessingWrapper::GetConfig() const
{
    if (_impl) {
        return _impl->GetConfig();
    }
    return {};
}

} // namespace LiveKitCpp
