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
#include "Listener.h"
#include <api/data_channel_interface.h>
#include <memory>

namespace LiveKitCpp
{

class DataChannelListener;

// wrapper with support of more convenient
// callback in comparing with WebRTC stock
class DataChannel : public webrtc::RefCountInterface
{
    struct Impl;
public:
    static webrtc::scoped_refptr<DataChannel> create(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel);
    // Amount of bytes that can be queued for sending on the data channel.
    // Those are bytes that have not yet been processed at the SCTP level.
    static uint64_t maxSendQueueSize();
    
    static std::string lossyDCLabel() { return "_lossy"; }
    
    static std::string reliableDCLabel() { return "_reliable"; }
    
    ~DataChannel() override;
    // Used to receive events from the data channel. Only one listener can be
    // registered at a time. Unregister listener should be called before the
    // listener object is destroyed.
    void setListener(DataChannelListener* listener = nullptr);
    // The label attribute represents a label that can be used to distinguish this
    // DataChannel object from other DataChannel objects.
    std::string label() const;
    // The accessors below simply return the properties from the DataChannelInit
    // the data channel was constructed with.
    bool reliable() const;
    // TODO(deadbeef): Remove these dummy implementations when all classes have
    // implemented these APIs. They should all just return the values the
    // DataChannel was created with.
    bool ordered() const;
    std::optional<int> maxRetransmitsOpt() const;
    std::optional<int> maxPacketLifeTime() const;
    std::string protocol() const;
    bool negotiated() const;

    // Returns the ID from the DataChannelInit, if it was negotiated out-of-band.
    // If negotiated in-band, this ID will be populated once the DTLS role is
    // determined, and until then this will return -1.
    int id() const;
    webrtc::PriorityValue priority() const;
    webrtc::DataChannelInterface::DataState state() const;
    // When state is kClosed, and the DataChannel was not closed using
    // the closing procedure, returns the error information about the closing.
    // The default implementation returns "no error".
    webrtc::RTCError error() const;
    uint32_t messagesSent() const;
    uint64_t bytesSent() const;
    uint32_t messagesReceived() const;
    uint64_t bytesReceived() const;
    
    // Returns the number of bytes of application data (UTF-8 text and binary
    // data) that have been queued using Send but have not yet been processed at
    // the SCTP level. See comment above Send below.
    // Values are less or equal to MaxSendQueueSize().
    uint64_t bufferedAmount() const;

    // Begins the graceful data channel closing procedure. See:
    // https://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-13#section-6.7
    void close();

    // Queues up an asynchronus send operation to run on a network thread.
    // Once the operation has completed the `on_complete` callback is invoked,
    // on the thread the send operation was done on. It's important that
    // `on_complete` implementations do not block the current thread but rather
    // post any expensive operations to other worker threads.
    void Send(const webrtc::DataBuffer& buffer);
protected:
    DataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel);
private:
    const std::shared_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
