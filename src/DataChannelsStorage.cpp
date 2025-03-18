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
#include "DataChannel.h"
#include "DataPublishOptions.h"
#include "ProtoMarshaller.h"
#include "livekit_models.pb.h"
#include "Blob.h"

namespace LiveKitCpp
{

DataChannelsStorage::DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
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
        }
    }
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

bool DataChannelsStorage::sendUserPacket(const Bricks::Blob& data,
                                         const DataPublishOptions& options) const
{
    if (data) {
        auto sid = _sid();
        if (!sid.empty()) {
            const auto& label = DataChannel::label(options._reliable);
            if (const auto dc = get(label)) {
                livekit::UserPacket packet;
                packet.set_participant_sid(std::move(sid));
                packet.set_topic(options._topic);
                //packet.set_destination_identities(options._destinationIdentities);
                return send(dc, options._reliable, &livekit::DataPacket::mutable_user,
                            std::move(packet));
            }
        }
    }
    return false;
}

template <class TSetMethod, class TObject>
bool DataChannelsStorage::send(const rtc::scoped_refptr<DataChannel>& dc,
                               bool reliable,
                               const TSetMethod& setMethod,
                               TObject object) const
{
    if (dc) {
        auto identity = _identity();
        if (!identity.empty()) {
            livekit::DataPacket packet;
            if (const auto target = (packet.*setMethod)()) {
                *target = std::move(object);
                if (reliable) {
                    packet.set_kind(livekit::DataPacket_Kind_RELIABLE);
                }
                else {
                    packet.set_kind(livekit::DataPacket_Kind_LOSSY);
                }
                packet.set_participant_identity(std::move(identity));
                const auto bytes = ProtoMarshaller(logger().get()).toBytes(packet);
                if (const auto size = bytes.size()) {
                    const rtc::CopyOnWriteBuffer data(bytes.data(), size);
                    dc->send(webrtc::DataBuffer(data, true));
                    return true;
                }
            }
        }
    }
    return false;
}

void DataChannelsStorage::onStateChange(DataChannel* channel)
{
    if (channel && canLogVerbose()) {
        logVerbose(dcType(channel->local()) + " data channel '" +
                         channel->label() + "' state has been changed to " +
                         dataStateToString(channel->state()));
    }
}

void DataChannelsStorage::onMessage(DataChannel* channel, const webrtc::DataBuffer& /*buffer*/)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose("a message buffer was successfully received for '" +
                       channel->label() + "' " + dcType(channel->local()) +
                       " data channel");
        }
        if (_listener) { // parse
            
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
        if (canLogError()) {
            logError("send operation failed for '" + channel->label() + "' " +
                     dcType(channel->local()) +
                     " data channel: " + error.message());
        }
        _listener.invoke(&DataExchangeListener::onSendError, channel, std::move(error));
    }
}

} // namespace LiveKitCpp
