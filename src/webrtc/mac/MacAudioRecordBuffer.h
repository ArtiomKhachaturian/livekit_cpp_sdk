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
#ifdef __APPLE__
#include <rtc_base/buffer.h>
#include <CoreAudio/CoreAudio.h>

namespace LiveKitCpp
{

class MacAudioRecordBuffer
{
public:
    MacAudioRecordBuffer(UInt32 bytesPerFrame, UInt32 numberChannels = 1U);
    void SetFramesCount(UInt32 framesCount);
    const int16_t* GetData() const { return _storage.data(); }
    operator rtc::ArrayView<const int16_t> () const { return _storage; }
    operator const AudioBufferList* () const noexcept { return &_buffers; }
    operator AudioBufferList* () noexcept { return &_buffers; }
private:
    void SetSize(size_t size);
private:
    const UInt32 _bytesPerFrame;
    rtc::BufferT<int16_t, true> _storage;
    AudioBufferList _buffers;
};

} // namespace LiveKitCpp

#endif
