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
#pragma once // DataExchangeListener.h
#include <cstdint>
#include <api/rtc_error.h>


namespace LiveKitCpp
{

class DataChannel;

class DataExchangeListener
{
public:
    virtual void onSendError(DataChannel* /*channel*/, webrtc::RTCError /*error*/) {}
    //virtual void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) = 0;
protected:
    virtual ~DataExchangeListener() = default;
};

} // namespace LiveKitCpp
