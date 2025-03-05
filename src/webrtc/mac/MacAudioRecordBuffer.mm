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
#include "MacAudioRecordBuffer.h"
#include <rtc_base/checks.h>

namespace LiveKitCpp
{

MacAudioRecordBuffer::MacAudioRecordBuffer(UInt32 bytesPerFrame, UInt32 numberChannels)
    : _bytesPerFrame(bytesPerFrame)
{
    RTC_DCHECK(_bytesPerFrame);
    RTC_DCHECK(numberChannels);
    _buffers.mNumberBuffers = 1U;
    _buffers.mBuffers[0].mData = nullptr;
    _buffers.mBuffers[0].mDataByteSize = 0U;
    _buffers.mBuffers[0].mNumberChannels = numberChannels;
}

void MacAudioRecordBuffer::SetFramesCount(UInt32 framesCount)
{
    SetSize(framesCount * _bytesPerFrame);
}

void MacAudioRecordBuffer::SetSize(size_t size)
{
    // Allocate AudioBuffers to be used as storage for the received audio.
    // The AudioBufferList structure works as a placeholder for the
    // AudioBuffer structure, which holds a pointer to the actual data buffer
    // in `_storage`. Recorded audio will be rendered into this memory
    // at each input callback when calling AudioUnitRender().
    _storage.Clear();
    _storage.SetSize(size);
    _buffers.mBuffers[0].mData = _storage.data();
    _buffers.mBuffers[0].mDataByteSize = static_cast<UInt32>(_storage.size());
}

} // namespace LiveKitCpp
