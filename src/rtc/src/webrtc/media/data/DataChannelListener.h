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
#pragma once // DataChannelListener.h
#include <cstdint>
#include <api/rtc_error.h>

namespace webrtc {
struct DataBuffer;
}

namespace LiveKitCpp
{

class DataChannel;

class DataChannelListener
{
public:
    // The data channel state have changed.
    virtual void onStateChange(DataChannel* channel) = 0;
    //  A data buffer was successfully received.
    virtual void onMessage(DataChannel* channel,
                           const webrtc::DataBuffer& buffer) = 0;
    // The data channel's buffered_amount has changed.
    virtual void onBufferedAmountChange(DataChannel* /*channel*/,
                                        uint64_t /*sentDataSize*/) {}
    virtual void onSendError(DataChannel* /*channel*/, webrtc::RTCError /*error*/) {}
protected:
    virtual ~DataChannelListener() = default;
};

} // namespace LiveKitCpp
