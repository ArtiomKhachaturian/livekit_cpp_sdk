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
#include "Loggable.h"
#include "DataChannelListener.h"
#include "DataChannel.h"
#include "SafeObj.h"
#include <unordered_map>
#include <api/scoped_refptr.h>

namespace LiveKitCpp
{

template<class TBase = void>
class DataChannelsStorage : public Bricks::LoggableS<DataChannelListener, TBase>
{
    // key is channel label
    using DataChannels = std::unordered_map<std::string, rtc::scoped_refptr<DataChannel>>;
    using Base = Bricks::LoggableS<DataChannelListener, TBase>;
public:
    ~DataChannelsStorage() override { clear(); }
    bool add(rtc::scoped_refptr<DataChannel> channel);
    bool remove(const std::string& label);
    bool remove(const rtc::scoped_refptr<DataChannel>& channel);
    void clear();
protected:
    DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    static std::string dcType(bool local) { return local ? "local" : "remote"; }
private:
    Bricks::SafeObj<DataChannels> _dataChannels;
};


template<class TBase>
inline DataChannelsStorage<TBase>::DataChannelsStorage(const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
{
}

template<class TBase>
inline bool DataChannelsStorage<TBase>::add(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        const auto label = channel->label();
        const auto local = channel->local();
        if (label.empty()) {
            Base::logWarning("unnamed " + dcType(local) + " data channel, processing denied");
        }
        else {
            LOCK_WRITE_SAFE_OBJ(_dataChannels);
            const auto it = _dataChannels->find(label);
            if (it == _dataChannels->end()) {
                _dataChannels->insert(std::make_pair(label, std::move(channel)));
                Base::logVerbose(dcType(local) + " data channel '" + label + "' was added for observation");
            }
            else {
                Base::logWarning(dcType(local) + " data channel '" + label + "' is already present but will be overwritten");
                it->second = std::move(channel);
            }
            channel->setListener(this);
        }
    }
}

template<class TBase>
inline bool DataChannelsStorage<TBase>::remove(const std::string& label)
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

template<class TBase>
inline bool DataChannelsStorage<TBase>::remove(const rtc::scoped_refptr<DataChannel>& channel)
{
    return channel && remove(channel->label());
}

template<class TBase>
inline void DataChannelsStorage<TBase>::clear()
{
    LOCK_WRITE_SAFE_OBJ(_dataChannels);
    for (auto it = _dataChannels->begin(); it != _dataChannels->end(); ++it) {
        it->second->setListener(nullptr);
    }
    _dataChannels->clear();
}

} // namespace LiveKitCpp
