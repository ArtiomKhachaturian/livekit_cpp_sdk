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
#include "SafeObjAliases.h"
#include "livekit/rtc/media/WavFramesWriter.h"
#include <common_audio/wav_file.h>

namespace LiveKitCpp
{

struct WavFramesWriter::Impl
{
    const std::string _filename;
    Bricks::SafeUniquePtr<webrtc::WavWriter> _writer;
    Impl(std::string filename);
    ~Impl() { _writer({}); }
};

WavFramesWriter::WavFramesWriter(std::string filename)
    : _impl(std::make_unique<Impl>(std::move(filename)))
{
}

WavFramesWriter::~WavFramesWriter()
{
}

void WavFramesWriter::onData(const int16_t* audioData, int /*bitsPerSample*/,
                             int sampleRate, size_t numberOfChannels,
                             size_t numberOfFrames,
                             const std::optional<int64_t>& absoluteCaptureTimestampMs)
{
    if (audioData && sampleRate && numberOfChannels && numberOfFrames && !_impl->_filename.empty()) {
        LOCK_WRITE_SAFE_OBJ(_impl->_writer);
        if (!_impl->_writer->get()) {
            _impl->_writer = std::make_unique<webrtc::WavWriter>(_impl->_filename, sampleRate, numberOfChannels);
        }
        _impl->_writer->get()->WriteSamples(audioData, numberOfFrames * numberOfChannels);
    }
}

WavFramesWriter::Impl::Impl(std::string filename)
    : _filename(std::move(filename))
{
}

} // namespace LiveKitCpp
