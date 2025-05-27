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
#pragma once // AudioProcessingWrapper.h
#include <api/audio/audio_processing.h>
#include <modules/audio_processing/include/aec_dump.h>
#include <atomic>

namespace LiveKitCpp
{

class AudioProcessingWrapper : public webrtc::AudioProcessing
{
public:
    int Initialize() override;
    int Initialize(const webrtc::ProcessingConfig& processingConfig) override;
    void ApplyConfig(const webrtc::AudioProcessing::Config& config) override;
    bool CreateAndAttachAecDump(absl::string_view fileName, int64_t maxLogSizeBytes,
                                absl::Nonnull<webrtc::TaskQueueBase*> workerQueue) override;
    bool CreateAndAttachAecDump(FILE* handle, int64_t maxLogSizeBytes,
                                absl::Nonnull<webrtc::TaskQueueBase*> workerQueue) override;
    void AttachAecDump(std::unique_ptr<webrtc::AecDump> aecDump) override;
    void DetachAecDump() override;
    void SetRuntimeSetting(RuntimeSetting setting) override;
    bool PostRuntimeSetting(RuntimeSetting setting) override;
    int ProcessStream(const int16_t* const src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      int16_t* const dest) override;
    int ProcessStream(const float* const* src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      float* const* dest) override;
    bool GetLinearAecOutput(rtc::ArrayView<std::array<float, 160>> linearOutput) const override;
    void set_output_will_be_muted(bool muted) override;
    int set_stream_delay_ms(int delay) override;
    void set_stream_key_pressed(bool keyPressed) override;
    void set_stream_analog_level(int level) override;
    int recommended_stream_analog_level() const override;
    int ProcessReverseStream(const int16_t* const src,
                             const webrtc::StreamConfig& inputConfig,
                             const webrtc::StreamConfig& outputConfig,
                             int16_t* const dest) override;
    int AnalyzeReverseStream(const float* const* data,
                             const webrtc::StreamConfig& reverseConfig) override;
    int ProcessReverseStream(const float* const* src,
                             const webrtc::StreamConfig& inputConfig,
                             const webrtc::StreamConfig& outputConfig,
                             float* const* dest) override;
    int proc_sample_rate_hz() const override;
    int proc_split_sample_rate_hz() const override;
    size_t num_input_channels() const override;
    size_t num_proc_channels() const override;
    size_t num_output_channels() const override;
    size_t num_reverse_channels() const override;
    int stream_delay_ms() const override;
    webrtc::AudioProcessingStats GetStatistics(bool hasRemoteTracks) override;
    webrtc::AudioProcessingStats GetStatistics() override;
    webrtc::AudioProcessing::Config GetConfig() const override;
protected:
    AudioProcessingWrapper(webrtc::scoped_refptr<webrtc::AudioProcessing> impl);
private:
    const webrtc::scoped_refptr<webrtc::AudioProcessing> _impl;
};

} // namespace LiveKitCpp
