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
#ifdef WEBRTC_AVAILABLE
#include "DataExchangeListener.h"
#include "DataChannel.h"
#include "ProtoUtils.h"
#include "Utils.h"
#include "MarshalledTypesFwd.h"
#include "livekit_models.pb.h"
#include <nlohmann/json.hpp>
#include <rtc_base/time_utils.h>

namespace {

inline livekit::UserPacket makeUserPacket(std::string payload,
                                          std::string sid,
                                          const std::string& topic = {},
                                          const std::vector<std::string>& destinationIdentities = {}) {
    livekit::UserPacket packet;
    packet.set_payload(std::move(payload));
    packet.set_participant_sid(std::move(sid));
    packet.set_topic(topic);
    LiveKitCpp::toProtoRepeated(destinationIdentities,
                                packet.mutable_destination_sids());
    return packet;
}

struct NonCompliantChatMessage // usually from Swift client
{
    std::string messageId;
    //std::string senderIdentity;
    std::string text;
    //std::string senderSid;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NonCompliantChatMessage, messageId, text)
};

}

namespace LiveKitCpp
{

MARSHALLED_TYPE_NAME_DECL(livekit::DataPacket)
MARSHALLED_TYPE_NAME_DECL(livekit::UserPacket)
MARSHALLED_TYPE_NAME_DECL(livekit::ChatMessage)

struct DataChannelsStorage::ChatMessage
{
    std::string id;
    std::string message;
    int64_t timestamp = {};
    bool ignore = false;
    bool deleted = false;
    bool generated = false;
    ChatMessage() = default;
    ChatMessage(std::string id, std::string message, int64_t timestamp = rtc::TimeMillis());
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ChatMessage, id, message, timestamp, ignore)
};

DataChannelsStorage::DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger,
                                         std::string logCategory)
    : Bricks::LoggableS<DataChannelListener>(logger)
    , _logCategory(std::move(logCategory))
{
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
            channel->setListener(this);
            LOCK_WRITE_SAFE_OBJ(_dataChannels);
            const auto it = _dataChannels->find(label);
            if (it == _dataChannels->end()) {
                _dataChannels->insert(std::make_pair(label, std::move(channel)));
                logVerbose(dcType(local) + " data channel '" + label + "' was added for observation");
            }
            else {
                logWarning(dcType(local) + " data channel '" + label + "' is already present but will be overwritten");
                if (it->second) {
                    it->second->setListener(nullptr);
                }
                it->second = std::move(channel);
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
            it->second->setListener(nullptr);
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
        it->second->setListener(nullptr);
    }
    _dataChannels->clear();
}

rtc::scoped_refptr<DataChannel> DataChannelsStorage::get(const std::string& label) const
{
    if (!label.empty()) {
        LOCK_READ_SAFE_OBJ(_dataChannels);
        const auto it = _dataChannels->find(label);
        if (it != _dataChannels->end()) {
            return it->second;
        }
    }
    return {};
}

bool DataChannelsStorage::sendUserPacket(std::string payload, bool reliable,
                                         const std::vector<std::string>& destinationIdentities,
                                         const std::string& topic) const
{
    if (payload.empty()) {
        logError("failed to send user packet - empty payload");
        return false;
    }
    {
        LOCK_READ_SAFE_OBJ(_sid);
        if (_sid->empty()) {
            logError("failed to send user packet - unknown user sid");
            return {};
        }
    }
    if (const auto dc = getChannelForSend(reliable)) {
        auto packet = makeUserPacket(std::move(payload), _sid(),
                                     topic, destinationIdentities);
        return send(dc, &livekit::DataPacket::mutable_user, std::move(packet));
    }
    return false;
}

bool DataChannelsStorage::sendChatMessage(std::string message, bool deleted) const
{
    if (message.empty()) {
        logError("failed to send chat message - message text is empty");
        return false;
    }
    if (const auto dc = getChannelForSend(true)) {
        livekit::ChatMessage chatMessage;
        chatMessage.set_message(std::move(message));
        chatMessage.set_timestamp(rtc::TimeMillis());
        chatMessage.set_id(makeUuid());
        chatMessage.set_deleted(deleted);
        // true if the chat message has been generated by an agent from a participant's audio transcription
        chatMessage.set_generated(false);
        return send(dc, &livekit::DataPacket::mutable_chat_message, std::move(chatMessage));
    }
    return false;
}

std::optional<DataChannelsStorage::ChatMessage> DataChannelsStorage::
    maybeChatMessage(const livekit::UserPacket& packet)
{
    const auto& payload = packet.payload();
    if (!payload.empty()) {
        try {
            // standard chat packet
            return nlohmann::json::parse(payload).get<ChatMessage>();
        }
        catch(const std::exception&) { /* ignore JSON parser errors */ }
        try {
            auto message = nlohmann::json::parse(payload).get<NonCompliantChatMessage>();
            return ChatMessage(std::move(message.messageId), std::move(message.text));
        }
        catch(const std::exception&) { /* ignore JSON parser errors */ }
    }
    return std::nullopt;
}

rtc::scoped_refptr<DataChannel> DataChannelsStorage::getChannelForSend(bool reliable) const
{
    {
        LOCK_READ_SAFE_OBJ(_identity);
        if (_identity->empty()) {
            logError("failed to send data - unknown user identity");
            return {};
        }
    }
    const auto& label = DataChannel::label(reliable);
    const auto dc = get(label);
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

template <class TSetMethod, class TObject>
bool DataChannelsStorage::send(const rtc::scoped_refptr<DataChannel>& dc,
                               const TSetMethod& setMethod,
                               TObject object) const
{
    if (dc) {
        auto identity = _identity();
        if (identity.empty()) {
            logError("failed to send data - unknown user identity");
            return false;
        }
        livekit::DataPacket packet;
        if (const auto target = (packet.*setMethod)()) {
            *target = std::move(object);
            if (dc->reliable()) {
                packet.set_kind(livekit::DataPacket_Kind_RELIABLE);
            }
            else {
                packet.set_kind(livekit::DataPacket_Kind_LOSSY);
            }
            packet.set_participant_identity(std::move(identity));
            const auto bytes = protoToBytes(packet, logger().get(), logCategory());
            if (const auto size = bytes.size()) {
                const rtc::CopyOnWriteBuffer data(bytes.data(), size);
                dc->send(webrtc::DataBuffer(data, true));
                return true;
            }
        }
        else {
            logError("proto method not available for set of '" +
                     marshalledTypeName<TObject>() + "'");
        }
    }
    return false;
}

void DataChannelsStorage::handle(const std::string& senderIdentity,
                                 const livekit::UserPacket& packet)
{
    bool defaultCallback = true;
    if (const auto chatMessage = maybeChatMessage(packet)) {
        if (updateLastChatMessageId(chatMessage->id)) {
            handle(senderIdentity, chatMessage.value());
        }
        defaultCallback = false;
    }
    if (defaultCallback) {
        _listener.invoke(&DataExchangeListener::onUserPacket,
                         packet.participant_sid(),
                         packet.participant_identity(),
                         packet.payload(),
                         fromProtoRepeated<std::string>(packet.destination_sids()),
                         packet.topic());
    }
}

void DataChannelsStorage::handle(const std::string& senderIdentity,
                                 const livekit::ChatMessage& message)
{
    if (updateLastChatMessageId(message.id())) {
        ChatMessage msg(message.id(), message.message(), message.timestamp());
        msg.deleted = message.deleted();
        msg.generated = message.generated();
        handle(senderIdentity, msg);
    }
}

void DataChannelsStorage::handle(const std::string& senderIdentity, const ChatMessage& message)
{
    _listener.invoke(&DataExchangeListener::onChatMessage,
                     senderIdentity,
                     message.message,
                     message.id,
                     message.timestamp,
                     message.deleted,
                     message.generated);
}

bool DataChannelsStorage::updateLastChatMessageId(const std::string& id)
{
    LOCK_WRITE_SAFE_OBJ(_lastChatMessageId);
    const auto prev = _lastChatMessageId.exchange(id);
    return prev.empty() || prev != id;
}

void DataChannelsStorage::onStateChange(DataChannel* channel)
{
    if (channel && canLogVerbose()) {
        logVerbose(dcType(channel->local()) + " data channel '" +
                          channel->label() + "' state has been changed to " +
                          dataStateToString(channel->state()));
    }
}

void DataChannelsStorage::onMessage(DataChannel* channel,
                                    const webrtc::DataBuffer& buffer)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose("a message buffer was successfully received for '" +
                       channel->label() + "' " + dcType(channel->local()) +
                       " data channel");
        }
        if (_listener) { // parse
            if (buffer.binary) {
                using namespace livekit;
                const auto data = buffer.data.data();
                const auto size = buffer.data.size();
                const auto packet = protoFromBytes<DataPacket>(data, size,
                                                               logger().get(),
                                                               logCategory());
                if (packet.has_value()) {
                    switch (packet->value_case()) {
                        case livekit::DataPacket::kUser:
                            handle(packet->participant_identity(), packet->user());
                            break;
                        case livekit::DataPacket::kSpeaker:
                            break;
                        case livekit::DataPacket::kSipDtmf:
                            break;
                        case livekit::DataPacket::kTranscription:
                            break;
                        case livekit::DataPacket::kMetrics:
                            break;
                        case livekit::DataPacket::kChatMessage:
                            handle(packet->participant_identity(), packet->chat_message());
                            break;
                        case livekit::DataPacket::kRpcRequest:
                            break;
                        case livekit::DataPacket::kRpcAck:
                            break;
                        case livekit::DataPacket::kRpcResponse:
                            break;
                        case livekit::DataPacket::kStreamHeader:
                            break;
                        case livekit::DataPacket::kStreamChunk:
                            break;
                        case livekit::DataPacket::kStreamTrailer:
                            break;
                        default:
                            break;
                    }
                }
            }
            else {
                
            }
        }
    }
}

void DataChannelsStorage::onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose(dcType(channel->local()) + " data channel '" +
                       channel->label() + "' buffer amout has been changed to " +
                       std::to_string(sentDataSize) + " bytes");
        }
    }
}

void DataChannelsStorage::onSendError(DataChannel* channel, webrtc::RTCError error)
{
    if (channel) {
        const auto label = channel->label();
        if (canLogError()) {
            logError("send operation failed for '" + label + "' " +
                     dcType(channel->local()) +
                     " data channel: " + error.message());
        }
        _listener.invoke(&DataExchangeListener::onSendError, label, std::move(error));
    }
}

DataChannelsStorage::ChatMessage::ChatMessage(std::string id, std::string message,
                                              int64_t timestamp)
{
    this->id = std::move(id);
    this->message = std::move(message);
    this->timestamp = timestamp;
}

} // namespace LiveKitCpp
#endif
