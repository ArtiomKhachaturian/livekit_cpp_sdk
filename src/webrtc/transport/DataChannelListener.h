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

namespace webrtc {
struct DataBuffer;
}

namespace LiveKitCpp
{

enum class DataChannelType;

class DataChannelListener
{
public:
    // The data channel state have changed.
    virtual void onStateChange(DataChannelType channelType) = 0;
    //  A data buffer was successfully received.
    virtual void onMessage(DataChannelType channelType,
                           const webrtc::DataBuffer& buffer) = 0;
    // The data channel's buffered_amount has changed.
    virtual void onBufferedAmountChange(DataChannelType /*channelType*/,
                                        uint64_t /*sentDataSize*/) {}
    // Override this to get callbacks directly on the network thread.
    // An implementation that does that must not block the network thread
    // but rather only use the callback to trigger asynchronous processing
    // elsewhere as a result of the notification.
    // The default return value, `false`, means that notifications will be
    // delivered on the signaling thread associated with the peerconnection
    // instance.
    // TODO(webrtc:11547): Eventually all DataChannelObserver implementations
    // should be called on the network thread and this method removed.
    virtual bool isOkToCallOnTheNetworkThread(DataChannelType /*channelType*/) { return false; }
protected:
    virtual ~DataChannelListener() = default;
};

} // namespace LiveKitCpp
