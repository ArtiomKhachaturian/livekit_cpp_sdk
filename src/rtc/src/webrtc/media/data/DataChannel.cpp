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
#include "DataChannel.h"
#include "DataChannelListener.h"
#include <api/make_ref_counted.h>

namespace LiveKitCpp
{

struct DataChannel::Impl : public webrtc::DataChannelObserver
{
    DataChannel* const _self;
    const webrtc::scoped_refptr<webrtc::DataChannelInterface> _channel;
    const bool _local;
    Bricks::Listener<DataChannelListener*> _listener;
    Impl(DataChannel* self, bool local,
         webrtc::scoped_refptr<webrtc::DataChannelInterface> channel);
    // impl. of webrtc::DataChannelObserver
    void OnStateChange() final;
    void OnMessage(const webrtc::DataBuffer& buffer) final;
    void OnBufferedAmountChange(uint64_t sentDataSize) final;
    bool IsOkToCallOnTheNetworkThread() final { return true; }
};

DataChannel::DataChannel(bool local, webrtc::scoped_refptr<webrtc::DataChannelInterface> channel)
    : _impl(std::make_shared<Impl>(this, local, std::move(channel)))
{
}

DataChannel::~DataChannel()
{
    close();
    setListener(nullptr);
}

bool DataChannel::local() const noexcept
{
    return _impl->_local;
}

webrtc::scoped_refptr<DataChannel> DataChannel::
    create(bool local, webrtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (channel) {
        return webrtc::make_ref_counted<DataChannel>(local, std::move(channel));
    }
    return {};
}

uint64_t DataChannel::maxSendQueueSize()
{
    return webrtc::DataChannelInterface::MaxSendQueueSize();
}

const std::string& DataChannel::lossyLabel()
{
    static const std::string label("_lossy");
    return label;
}

const std::string& DataChannel::reliableLabel()
{
    static const std::string label("_reliable");
    return label;
}

const std::string& DataChannel::label(bool reliable)
{
    return reliable ? reliableLabel() : lossyLabel();
}

void DataChannel::setListener(DataChannelListener* listener)
{
    if (listener) {
        _impl->_listener.set(listener);
        _impl->_channel->RegisterObserver(_impl.get());
    }
    else {
        _impl->_channel->UnregisterObserver();
        _impl->_listener.reset();
    }
}

std::string DataChannel::label() const
{
    return _impl->_channel->label();
}

bool DataChannel::reliable() const
{
    return _impl->_channel->reliable();
}

bool DataChannel::ordered() const
{
    return _impl->_channel->ordered();
}

std::optional<int> DataChannel::maxRetransmitsOpt() const
{
    return _impl->_channel->maxRetransmitsOpt();
}

std::optional<int> DataChannel::maxPacketLifeTime() const
{
    return _impl->_channel->maxPacketLifeTime();
}

std::string DataChannel::protocol() const
{
    return _impl->_channel->protocol();
}

bool DataChannel::negotiated() const
{
    return _impl->_channel->negotiated();
}

int DataChannel::id() const
{
    return _impl->_channel->id();
}

webrtc::PriorityValue DataChannel::priority() const
{
    return _impl->_channel->priority();
}

webrtc::DataChannelInterface::DataState DataChannel::state() const
{
    return _impl->_channel->state();
}

webrtc::RTCError DataChannel::error() const
{
    return _impl->_channel->error();
}

uint32_t DataChannel::messagesSent() const
{
    return _impl->_channel->messages_sent();
}

uint64_t DataChannel::bytesSent() const
{
    return _impl->_channel->bytes_sent();
}

uint32_t DataChannel::messagesReceived() const
{
    return _impl->_channel->messages_received();
}

uint64_t DataChannel::bytesReceived() const
{
    return _impl->_channel->bytes_received();
}

uint64_t DataChannel::bufferedAmount() const
{
    return _impl->_channel->buffered_amount();
}

void DataChannel::close()
{
    _impl->_channel->Close();
}

void DataChannel::send(const webrtc::DataBuffer& buffer)
{
    _impl->_channel->SendAsync(buffer, [ref = std::weak_ptr<Impl>()](webrtc::RTCError error){
        if (const auto impl = ref.lock()) {
            if (!error.ok()) {
                impl->_listener.invoke(&DataChannelListener::onSendError,
                                       impl->_self, std::move(error));
            }
        }
    });
}

DataChannel::Impl::Impl(DataChannel* self, bool local,
                        webrtc::scoped_refptr<webrtc::DataChannelInterface> channel)
    : _self(self)
    , _local(local)
    , _channel(std::move(channel))
{
}

void DataChannel::Impl::OnStateChange()
{
    _listener.invoke(&DataChannelListener::onStateChange, _self);
}

void DataChannel::Impl::OnMessage(const webrtc::DataBuffer& buffer)
{
    _listener.invoke(&DataChannelListener::onMessage, _self, buffer);
}

void DataChannel::Impl::OnBufferedAmountChange(uint64_t sentDataSize)
{
    _listener.invoke(&DataChannelListener::onBufferedAmountChange, _self, sentDataSize);
}

std::string dataStateToString(webrtc::DataChannelInterface::DataState state)
{
    return webrtc::DataChannelInterface::DataStateString(state);
}

} // namespace LiveKitCpp
