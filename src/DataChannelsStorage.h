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
#include "DataChannelListener.h"
#include "DataChannel.h"
#include "SafeObj.h"
#include <unordered_map>
#include <api/scoped_refptr.h>

namespace LiveKitCpp
{

template<class... BaseInterfaces>
class DataChannelsStorage : public Bricks::LoggableS<BaseInterfaces...>,
                            protected DataChannelListener
{
    // key is channel label
    using DataChannels = std::unordered_map<std::string, rtc::scoped_refptr<DataChannel>>;
    using Base = Bricks::LoggableS<BaseInterfaces...>;
public:
    ~DataChannelsStorage() override { clear(); }
    bool add(rtc::scoped_refptr<DataChannel> channel);
    bool remove(const std::string& label);
    bool remove(const rtc::scoped_refptr<DataChannel>& channel);
    void clear();
protected:
    DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger = {});
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) override;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) override;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) override;
    void onSendError(DataChannel* channel, webrtc::RTCError error) override;
private:
    static std::string dcType(bool local) { return local ? "local" : "remote"; }
private:
    Bricks::SafeObj<DataChannels> _dataChannels;
};

template <class... BaseInterfaces>
inline DataChannelsStorage<BaseInterfaces...>::
    DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger)
        : Base(logger)
{
}

template <class... BaseInterfaces>
inline bool DataChannelsStorage<BaseInterfaces...>::
    add(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        const auto label = channel->label();
        const auto local = channel->local();
        if (label.empty()) {
            Base::logWarning("unnamed " + dcType(local) + " data channel, processing denied");
        }
        else {
            channel->setListener(this);
            LOCK_WRITE_SAFE_OBJ(_dataChannels);
            const auto it = _dataChannels->find(label);
            if (it == _dataChannels->end()) {
                _dataChannels->insert(std::make_pair(label, std::move(channel)));
                Base::logVerbose(dcType(local) + " data channel '" + label + "' was added for observation");
            }
            else {
                Base::logWarning(dcType(local) + " data channel '" + label + "' is already present but will be overwritten");
                if (it->second) {
                    it->second->setListener(nullptr);
                }
                it->second = std::move(channel);
            }
        }
    }
}

template <class... BaseInterfaces>
inline bool DataChannelsStorage<BaseInterfaces...>::
    remove(const std::string& label)
{
    bool removed = false;
    if (!label.empty()) {
        LOCK_WRITE_SAFE_OBJ(_dataChannels);
        const auto it = _dataChannels->find(label);
        if (it != _dataChannels->end()) {
            const auto local = it->second->local();
            it->second->setListener(nullptr);
            _dataChannels->erase(it);
            Base::logVerbose(dcType(local) + " data channel '" + label + "' has been removed from observation");
            removed = true;
        }
        else {
            Base::logWarning("data channel '" + label + "' was not found");
        }
    }
    return removed;
}

template <class... BaseInterfaces>
inline bool DataChannelsStorage<BaseInterfaces...>::
    remove(const rtc::scoped_refptr<DataChannel>& channel)
{
    return channel && remove(channel->label());
}

template <class... BaseInterfaces>
inline void DataChannelsStorage<BaseInterfaces...>::clear()
{
    LOCK_WRITE_SAFE_OBJ(_dataChannels);
    for (auto it = _dataChannels->begin(); it != _dataChannels->end(); ++it) {
        it->second->setListener(nullptr);
    }
    _dataChannels->clear();
}

template <class... BaseInterfaces>
inline void DataChannelsStorage<BaseInterfaces...>::onStateChange(DataChannel* channel)
{
    if (channel && Base::canLogVerbose()) {
        Base::logVerbose(dcType(channel->local()) + " data channel '" +
                         channel->label() + "' state has been changed to " +
                         dataStateToString(channel->state()));
    }
}

template <class... BaseInterfaces>
inline void DataChannelsStorage<BaseInterfaces...>::
    onMessage(DataChannel* channel, const webrtc::DataBuffer& /*buffer*/)
{
    if (channel && Base::canLogVerbose()) {
        Base::logVerbose("a message buffer was successfully received for '" +
                         channel->label() + "' " + dcType(channel->local()) +
                         " data channel");
    }
}

template <class... BaseInterfaces>
inline void DataChannelsStorage<BaseInterfaces...>::
    onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize)
{
    if (channel && Base::canLogVerbose()) {
        Base::logVerbose(dcType(channel->local()) + " data channel '" +
                         channel->label() + "' buffer amout has been changed to " +
                         std::to_string(sentDataSize) + " bytes");
    }
}

template <class... BaseInterfaces>
inline void DataChannelsStorage<BaseInterfaces...>::
    onSendError(DataChannel* channel, webrtc::RTCError error)
{
    if (channel && Base::canLogError()) {
        Base::logError("send operation failed for '" + channel->label() + "' " +
                       dcType(channel->local()) +
                       " data channel: " + error.message());
    }
}

} // namespace LiveKitCpp
#endif
