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
#pragma once // DataChannelObserver.h
#include <api/data_channel_interface.h>

namespace LiveKitCpp
{

enum class DataChannelType;
class DataChannelListener;

class DataChannelObserver : public webrtc::DataChannelObserver
{
public:
    DataChannelObserver(DataChannelType channelType, DataChannelListener* listener);
    ~DataChannelObserver() override = default;
    // impl. of webrtc::DataChannelObserver
    void OnStateChange() final;
    void OnMessage(const webrtc::DataBuffer& buffer) final;
    void OnBufferedAmountChange(uint64_t sentDataSize) final;
    bool IsOkToCallOnTheNetworkThread() final;
private:
    const DataChannelType _channelType;
    DataChannelListener* _listener;
};

} // namespace LiveKitCpp
