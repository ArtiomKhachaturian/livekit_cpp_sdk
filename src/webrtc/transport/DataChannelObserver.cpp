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
#include "DataChannelObserver.h"
#include "DataChannelType.h"
#include "DataChannelListener.h"

namespace LiveKitCpp
{


DataChannelObserver::DataChannelObserver(DataChannelType channelType,
                                         DataChannelListener* listener)
    : _channelType(channelType)
    , _listener(listener)
{
}

void DataChannelObserver::OnStateChange()
{
    if (_listener) {
        _listener->onStateChange(_channelType);
    }
}

void DataChannelObserver::OnMessage(const webrtc::DataBuffer& buffer)
{
    if (_listener) {
        _listener->onMessage(_channelType, buffer);
    }
}

void DataChannelObserver::OnBufferedAmountChange(uint64_t sentDataSize)
{
    if (_listener) {
        _listener->onBufferedAmountChange(_channelType, sentDataSize);
    }
    webrtc::DataChannelObserver::OnBufferedAmountChange(sentDataSize);
}

bool DataChannelObserver::IsOkToCallOnTheNetworkThread()
{
    if (_listener) {
        return _listener->isOkToCallOnTheNetworkThread(_channelType);
    }
    return webrtc::DataChannelObserver::IsOkToCallOnTheNetworkThread();
}

std::string toString(DataChannelType type)
{
    switch (type) {
        case DataChannelType::Lossy:
            return "_lossy";
        case DataChannelType::Relaible:
            return "_reliable";
        default:
            break;
    }
    return "";
}

} // namespace LiveKitCpp
