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
#include "DataChannelsStorage.h"
#include "DataExchangeListener.h"
#include "DataChannelListener.h"
#include "Utils.h"
#include "livekit/signaling/SignalClient.h"
#include "livekit/signaling/sfu/ChatMessage.h"
#include "livekit/signaling/sfu/DataPacket.h"
#include "livekit/signaling/sfu/UserPacket.h"
#include <rtc_base/time_utils.h>

namespace {

inline std::string dcType(bool local) { return local ? "local" : "remote"; }

}

namespace LiveKitCpp
{

class DataChannelsStorage::Wrapper : public Bricks::LoggableS<DataChannelListener>
{
public:
    Wrapper(rtc::scoped_refptr<DataChannel> channel,
            SignalServerListener* listener,
            const std::shared_ptr<Bricks::Logger>& logger,
            const std::string& logCategory);
    ~Wrapper() final { close(); }
    void close();
    bool isOpen() const { return _channel && _channel->isOpen(); }
    bool local() const { return _channel && _channel->local(); }
    bool reliable() const { return _channel && _channel->reliable(); }
    bool sendDataPacket(DataPacket packet) const;
    bool sendChatMessage(std::string participantIdentity, ChatMessage mesage,
                         std::vector<std::string> destinationIdentities = {});
    bool sendUserPacket(std::string participantIdentity, UserPacket packet,
                        std::vector<std::string> destinationIdentities = {});
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) final;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) final;
    void onSendError(DataChannel* channel, webrtc::RTCError error) final;
protected:
    // overrides of Bricks::LoggableS<>
    std::string_view logCategory() const final { return _logCategory; }
private:
    template <class TValue>
    DataPacket createDataPacket(std::string participantIdentity, TValue value,
                                std::vector<std::string> destinationIdentities = {}) const;
private:
    const rtc::scoped_refptr<DataChannel> _channel;
    const std::string _logCategory;
    SignalClient _client;
};

DataChannelsStorage::DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger,
                                         std::string logCategory)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _logCategory(std::move(logCategory))
{
}

DataChannelsStorage::~DataChannelsStorage()
{
    clear();
}

void DataChannelsStorage::setListener(DataExchangeListener* listener)
{
    _listener = listener;
}

bool DataChannelsStorage::add(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        const auto label = channel->label();
        const auto local = channel->local();
        if (label.empty()) {
            logWarning("unnamed " + dcType(local) + " data channel, processing denied");
        }
        else {
            SignalServerListener* const listener = this;
            auto wrapper = std::make_shared<Wrapper>(std::move(channel),
                                                     listener,
                                                     logger(),
                                                     _logCategory);
            LOCK_WRITE_SAFE_OBJ(_dataChannels);
            const auto it = _dataChannels->find(label);
            if (it == _dataChannels->end()) {
                _dataChannels->insert(std::make_pair(label, std::move(wrapper)));
                logVerbose(dcType(local) + " data channel '" + label + "' was added for observation");
            }
            else {
                logWarning(dcType(local) + " data channel '" + label +
                           "' is already present but will be overwritten");
                it->second = std::move(wrapper);
            }
            return true;
        }
    }
    return false;
}

bool DataChannelsStorage::remove(const std::string& label)
{
    bool removed = false;
    if (!label.empty()) {
        LOCK_WRITE_SAFE_OBJ(_dataChannels);
        const auto it = _dataChannels->find(label);
        if (it != _dataChannels->end()) {
            const auto local = it->second->local();
            _dataChannels->erase(it);
            if (canLogVerbose()) {
                logVerbose(dcType(local) + " data channel '" + label +
                           "' has been removed from observation");
            }
            removed = true;
        }
        else if (canLogWarning()) {
            logWarning("data channel '" + label + "' was not found");
        }
    }
    return removed;
}

bool DataChannelsStorage::remove(const rtc::scoped_refptr<DataChannel>& channel)
{
    return channel && remove(channel->label());
}

void DataChannelsStorage::clear()
{
    LOCK_WRITE_SAFE_OBJ(_dataChannels);
    for (auto it = _dataChannels->begin(); it != _dataChannels->end(); ++it) {
        it->second->close();
    }
    _dataChannels->clear();
}

bool DataChannelsStorage::sendUserPacket(std::string payload, bool reliable, std::string topic,
                                         std::vector<std::string> destinationSids,
                                         std::vector<std::string> destinationIdentities) const
{
    if (payload.empty()) {
        logError("failed to send user packet - empty payload");
        return false;
    }
    auto sid = _sid();
    if (sid.empty()) {
        logError("failed to send user packet - unknown user sid");
        return {};
    }
    if (const auto dc = getChannelForSend(reliable)) {
        auto identity = _identity();
        UserPacket userPacket;
        userPacket._participantSid = std::move(sid);
        userPacket._participantIdentity = identity;
        userPacket._payload = std::move(payload);
        userPacket._topic = std::move(topic);
        userPacket._destinationSids = std::move(destinationSids);
        userPacket._destinationIdentities = destinationIdentities;
        return dc->sendUserPacket(std::move(identity), std::move(userPacket),
                                  std::move(destinationIdentities));
    }
    return false;
}

bool DataChannelsStorage::sendChatMessage(std::string message, bool deleted, bool generated,
                                          std::vector<std::string> destinationIdentities) const
{
    if (message.empty()) {
        logError("failed to send chat message - message text is empty");
        return false;
    }
    if (const auto dc = getChannelForSend(true)) {
        ChatMessage chatMessage;
        chatMessage._message = std::move(message);
        chatMessage._timestamp = rtc::TimeMillis();
        chatMessage._id = makeUuid();
        chatMessage._deleted = deleted;
        chatMessage._generated = generated;
        return dc->sendChatMessage(_identity(), std::move(chatMessage), std::move(destinationIdentities));
    }
    return false;
}

std::shared_ptr<DataChannelsStorage::Wrapper> DataChannelsStorage::
    getChannelForSend(bool reliable) const
{
    {
        LOCK_READ_SAFE_OBJ(_identity);
        if (_identity->empty()) {
            logError("failed to send data - unknown user identity");
            return {};
        }
    }
    
    const auto& label = DataChannel::label(reliable);
    std::shared_ptr<Wrapper> dc;
    if (!label.empty()) {
        LOCK_READ_SAFE_OBJ(_dataChannels);
        const auto it = _dataChannels->find(label);
        if (it != _dataChannels->end()) {
            dc = it->second;
        }
    }

    if (!dc) {
        logError("failed to send data - data channel '" + label + "' was not found");
        return {};
    }
    if (!dc->isOpen()) {
        logError("failed to send data - data channel '" + label + "' is not opened");
        return {};
    }
    return dc;
}

void DataChannelsStorage::onDataPacket(DataPacket packet)
{
    if (std::holds_alternative<UserPacket>(packet._value)) {
        _listener.invoke(&DataExchangeListener::onUserPacket,
                         std::move(std::get<UserPacket>(packet._value)),
                         std::move(packet._participantIdentity),
                         std::move(packet._destinationIdentities));
    }
    else if (std::holds_alternative<ChatMessage>(packet._value)) {
        _listener.invoke(&DataExchangeListener::onChatMessage,
                         std::move(std::get<ChatMessage>(packet._value)),
                         std::move(packet._participantIdentity),
                         std::move(packet._destinationIdentities));
    }
    else if (std::holds_alternative<DataStreamHeader>(packet._value)) {
        _listener.invoke(&DataExchangeListener::onDataStreamHeader,
                         std::move(std::get<DataStreamHeader>(packet._value)),
                         std::move(packet._participantIdentity),
                         std::move(packet._destinationIdentities));
    }
    else if (std::holds_alternative<DataStreamChunk>(packet._value)) {
        _listener.invoke(&DataExchangeListener::onDataStreamChunk,
                         std::move(std::get<DataStreamChunk>(packet._value)),
                         std::move(packet._participantIdentity),
                         std::move(packet._destinationIdentities));
    }
    else if (std::holds_alternative<DataStreamTrailer>(packet._value)) {
        _listener.invoke(&DataExchangeListener::onDataStreamTrailer,
                         std::move(std::get<DataStreamTrailer>(packet._value)),
                         std::move(packet._participantIdentity),
                         std::move(packet._destinationIdentities));
    }
}

void DataChannelsStorage::onSignalParseError(std::string details)
{
    _listener.invoke(&DataExchangeListener::onError, std::move(details));
}

DataChannelsStorage::Wrapper::Wrapper(rtc::scoped_refptr<DataChannel> channel,
                                      SignalServerListener* listener,
                                      const std::shared_ptr<Bricks::Logger>& logger,
                                      const std::string& logCategory)
    :  Bricks::LoggableS<DataChannelListener>(logger)
    , _channel(std::move(channel))
    , _logCategory(logCategory)
    , _client(_channel.get(), logger.get())
{
    _client.setServerListener(listener);
    if (_channel) {
        _channel->setListener(this);
    }
}

void DataChannelsStorage::Wrapper::close()
{
    if (_channel) {
        _channel->close();
        _channel->setListener(nullptr);
    }
    _client.setServerListener(nullptr);
}

bool DataChannelsStorage::Wrapper::sendDataPacket(DataPacket packet) const
{
    return _client.sendDataPacket(std::move(packet));
}

bool DataChannelsStorage::Wrapper::sendChatMessage(std::string participantIdentity,
                                                   ChatMessage mesage,
                                                   std::vector<std::string> destinationIdentities)
{
    if (_channel) {
        return sendDataPacket(createDataPacket(std::move(participantIdentity),
                                               std::move(mesage),
                                               std::move(destinationIdentities)));
    }
    return false;
}

bool DataChannelsStorage::Wrapper::sendUserPacket(std::string participantIdentity,
                                                  UserPacket packet,
                                                  std::vector<std::string> destinationIdentities)
{
    if (_channel) {
        return sendDataPacket(createDataPacket(std::move(participantIdentity),
                                               std::move(packet),
                                               std::move(destinationIdentities)));
    }
    return false;
}

void DataChannelsStorage::Wrapper::onStateChange(DataChannel* channel)
{
    if (channel && canLogVerbose()) {
        logVerbose(dcType(channel->local()) + " data channel '" +
                          channel->label() + "' state has been changed to " +
                          dataStateToString(channel->state()));
    }
}

void DataChannelsStorage::Wrapper::onMessage(DataChannel* channel,
                                             const webrtc::DataBuffer& buffer)
{
    if (channel && channel == _channel.get() && buffer.binary) {
        if (canLogVerbose()) {
            logVerbose("a message buffer was successfully received for '" +
                       channel->label() + "' " + dcType(channel->local()) +
                       " data channel");
        }
        const auto& data = buffer.data;
        _client.parseProtobufData(data.data(), data.size());
    }
}

void DataChannelsStorage::Wrapper::onBufferedAmountChange(DataChannel* channel,
                                                          uint64_t sentDataSize)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose(dcType(channel->local()) + " data channel '" +
                       channel->label() + "' buffer amout has been changed to " +
                       std::to_string(sentDataSize) + " bytes");
        }
    }
}

void DataChannelsStorage::Wrapper::onSendError(DataChannel* channel,
                                               webrtc::RTCError error)
{
    if (channel) {
        const auto label = channel->label();
        if (canLogError()) {
            logError("send operation failed for '" + label + "' " +
                     dcType(channel->local()) +
                     " data channel: " + error.message());
        }
        _client.notifyAboutError(error.message());
    }
}

template <class TValue>
DataPacket DataChannelsStorage::Wrapper::createDataPacket(std::string participantIdentity,
                                                          TValue value,
                                                          std::vector<std::string> destinationIdentities) const
{
    DataPacket dataPacket;
    dataPacket._kind = reliable() ? DataPacketKind::Reliable : DataPacketKind::Lossy;
    dataPacket._participantIdentity = std::move(participantIdentity);
    dataPacket._destinationIdentities = std::move(destinationIdentities);
    dataPacket._value = std::move(value);
    return dataPacket;
}

} // namespace LiveKitCpp
