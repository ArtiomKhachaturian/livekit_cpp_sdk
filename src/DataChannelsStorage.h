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
#pragma once // DataChannelsStorage.h
#ifdef WEBRTC_AVAILABLE
#include "Loggable.h"
#include "Listener.h"
#include "DataChannelListener.h"
#include "SafeObj.h"
#include <unordered_map>
#include <api/scoped_refptr.h>


namespace LiveKitCpp
{

class DataExchangeListener;

class DataChannelsStorage : public Bricks::LoggableS<>,
                            private DataChannelListener
{
    // key is channel label
    using DataChannels = std::unordered_map<std::string, rtc::scoped_refptr<DataChannel>>;
public:
    DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger = {});
    ~DataChannelsStorage() override { clear(); }
    void setSid(const std::string& sid) { _sid(sid); }
    void setIdentity(const std::string& identity) { _identity(identity); }
    void setListener(DataExchangeListener* listener = nullptr);
    bool add(rtc::scoped_refptr<DataChannel> channel);
    bool remove(const std::string& label);
    bool remove(const rtc::scoped_refptr<DataChannel>& channel);
    void clear();
    rtc::scoped_refptr<DataChannel> get(const std::string& label) const;
    bool sendUserPacket(std::string payload,
                        bool reliable = false,
                        const std::vector<std::string>& destinationIdentities = {},
                        std::string topic = {}) const;
private:
    bool hasIdentity() const noexcept;
    template <class TSetMethod, class TObject>
    bool send(const rtc::scoped_refptr<DataChannel>& channel,
              bool reliable, const TSetMethod& setMethod, TObject object) const;
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) final;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) final;
    void onSendError(DataChannel* channel, webrtc::RTCError error) final;
private:
    static std::string dcType(bool local) { return local ? "local" : "remote"; }
private:
    Bricks::SafeObj<DataChannels> _dataChannels;
    Bricks::Listener<DataExchangeListener*> _listener;
    // from owner
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _sid;
};

} // namespace LiveKitCpp
#endif
