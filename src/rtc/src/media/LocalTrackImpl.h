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
#pragma once // LocalTrack.h
#include "Logger.h"
#include "LocalTrackAccessor.h"
#include "SafeScopedRefPtr.h"
#include "TrackManager.h"
#include "Utils.h"
#include "livekit/signaling/sfu/AddTrackRequest.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include "livekit/rtc/media/Track.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include "livekit/rtc/media/NetworkPriority.h"
#include <api/rtp_transceiver_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

class LocalAudioTrack;
class LocalVideoTrack;

template <class TBaseImpl>
class LocalTrackImpl : public TBaseImpl, public LocalTrackAccessor
{
    static_assert(std::is_base_of_v<LocalAudioTrack, TBaseImpl> || std::is_base_of_v<LocalVideoTrack, TBaseImpl>);
public:
    ~LocalTrackImpl() override = default;
    void setFrameTransformer(rtc::scoped_refptr<webrtc::FrameTransformerInterface> transformer);
    // impl. of LocalTrack
    std::string cid() const final { return TBaseImpl::id(); }
    webrtc::MediaType mediaType() const final;
    void setRemoteSideMute(bool mute) override;
    void setSid(const std::string& sid) final;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> media() const;
    bool fillRequest(AddTrackRequest* request) const override;
    bool remoteMuted() const final { return _remoteMuted; }
    bool muted() const override { return TBaseImpl::muted(); }
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    std::string sid() const final { return _sid(); }
    EncryptionType encryption() const final { return _encryption; }
    std::string name() const final { return _name; }
    bool remote() const noexcept final { return false; }
    NetworkPriority networkPriority() const final { return _networkPriority; }
    void setNetworkPriority(NetworkPriority priority) final;
    double bitratePriority() const final { return _bitratePriority; }
    void setBitratePriority(double priority) final;
protected:
    template <class TMediaDevice>
    LocalTrackImpl(std::string name,
                   std::shared_ptr<TMediaDevice> mediaDevice,
                   EncryptionType encryption,
                   rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver,
                   const std::weak_ptr<TrackManager>& trackManager);
    virtual bool updateSenderInitialParameters(webrtc::RtpParameters& parameters) const;
    webrtc::RtpParameters rtpParameters() const;
    void setRtpParameters(const webrtc::RtpParameters& parameters);
    // overrides of TrackImpl<>
    void onMuteChanged(bool mute) const final;
private:
    rtc::scoped_refptr<webrtc::RtpSenderInterface> sender() const;
    static bool setBitratePriority(double priority, std::vector<webrtc::RtpEncodingParameters>& encodings);
    static bool setBitratePriority(double priority, webrtc::RtpParameters& parameters);
    static bool setNetworkPriority(NetworkPriority priority, std::vector<webrtc::RtpEncodingParameters>& encodings);
    static bool setNetworkPriority(NetworkPriority priority, webrtc::RtpParameters& parameters);
private:
    const std::string _name;
    const EncryptionType _encryption;
    const rtc::scoped_refptr<webrtc::RtpTransceiverInterface> _transceiver;
    std::atomic_bool _remoteMuted = false;
    Bricks::SafeObj<std::string> _sid;
    std::atomic<NetworkPriority> _networkPriority = NetworkPriority::Low;
    std::atomic<double> _bitratePriority = webrtc::kDefaultBitratePriority;
};

template <class TBaseImpl>
template <class TMediaDevice>
inline LocalTrackImpl<TBaseImpl>::LocalTrackImpl(std::string name,
                                                 std::shared_ptr<TMediaDevice> mediaDevice,
                                                 EncryptionType encryption,
                                                 rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver,
                                                 const std::weak_ptr<TrackManager>& trackManager)
    : TBaseImpl(std::move(mediaDevice), trackManager)
    , _name(std::move(name))
    , _encryption(encryption)
    , _transceiver(std::move(transceiver))
{
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setFrameTransformer(rtc::scoped_refptr<webrtc::FrameTransformerInterface> transformer)
{
    if (const auto sender = this->sender()) {
        sender->SetFrameTransformer(std::move(transformer));
    }
}

template <class TBaseImpl>
inline webrtc::MediaType LocalTrackImpl<TBaseImpl>::mediaType() const
{
    return _transceiver ? _transceiver->media_type() : webrtc::MediaType::UNSUPPORTED;
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setRemoteSideMute(bool mute)
{
    if (exchangeVal(mute, _remoteMuted)) {
        TBaseImpl::notify(&MediaEventsListener::onRemoteSideMuteChanged, TBaseImpl::id(), mute);
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setSid(const std::string& sid)
{
    if (exchangeVal(sid, _sid)) {
        TBaseImpl::notify(&MediaEventsListener::onSidChanged, TBaseImpl::id(), sid);
    }
}

template <class TBaseImpl>
inline webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalTrackImpl<TBaseImpl>::media() const
{
    if (const auto& md = TBaseImpl::mediaDevice()) {
        return md->track();
    }
    return {};
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::fillRequest(AddTrackRequest* request) const
{
    if (request && media()) {
        request->_encryption = encryption();
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
        return true;
    }
    return false;
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::queryStats() const
{
    if (const auto m = TBaseImpl::trackManager()) {
        m->queryStats(sender(), TBaseImpl::statsCollector());
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setNetworkPriority(NetworkPriority priority)
{
    if (exchangeVal(priority, _networkPriority)) {
        auto parameters = rtpParameters();
        if (setNetworkPriority(priority, parameters)) {
            setRtpParameters(parameters);
        }
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setBitratePriority(double priority)
{
    if (exchangeVal(priority, _bitratePriority)) {
        auto parameters = rtpParameters();
        if (setBitratePriority(priority, parameters)) {
            setRtpParameters(parameters);
        }
    }
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::updateSenderInitialParameters(webrtc::RtpParameters& parameters) const
{
    const auto b1 = setBitratePriority(bitratePriority(), parameters);
    const auto b2 = setNetworkPriority(networkPriority(), parameters);
    return b1 || b2;
}

template <class TBaseImpl>
inline webrtc::RtpParameters LocalTrackImpl<TBaseImpl>::rtpParameters() const
{
    if (const auto sender = this->sender()) {
        return sender->GetParameters();
    }
    return {};
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setRtpParameters(const webrtc::RtpParameters& parameters)
{
    if (const auto sender = this->sender()) {
        sender->SetParametersAsync(parameters, [sid = sid(),
                                                tm = std::weak_ptr<TrackManager>(TBaseImpl::trackManager())](webrtc::RTCError error) {
            if (!error.ok()) {
                if (const auto m = tm.lock()) {
                    m->notifyAboutSetRtpParametersFailure(sid, error.message());
                }
            }
        });
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::onMuteChanged(bool mute) const
{
    TBaseImpl::onMuteChanged(mute);
    if (const auto m = TBaseImpl::trackManager()) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            m->notifyAboutMuteChanges(sid, mute);
        }
    }
}

template <class TBaseImpl>
inline rtc::scoped_refptr<webrtc::RtpSenderInterface> LocalTrackImpl<TBaseImpl>::sender() const
{
    if (_transceiver) {
        return _transceiver->sender();
    }
    return {};
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::setBitratePriority(double priority,
                                                          std::vector<webrtc::RtpEncodingParameters>& encodings)
{
    bool changed = false;
    if (!encodings.empty()) {
        for (auto& encoding : encodings) {
            if (!floatsIsEqual(encoding.bitrate_priority, priority)) {
                encoding.bitrate_priority = priority;
                changed = true;
            }
        }
    }
    return changed;
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::setBitratePriority(double priority,
                                                          webrtc::RtpParameters& parameters)
{
    return setBitratePriority(priority, parameters.encodings);
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::setNetworkPriority(NetworkPriority priority,
                                                          std::vector<webrtc::RtpEncodingParameters>& encodings)
{
    bool changed = false;
    if (!encodings.empty()) {
        webrtc::Priority rtcPriority = webrtc::Priority::kLow;
        switch (priority) {
            case NetworkPriority::VeryLow:
                rtcPriority = webrtc::Priority::kVeryLow;
                break;
            case NetworkPriority::Low:
                break;
            case NetworkPriority::Medium:
                rtcPriority = webrtc::Priority::kMedium;
                break;
            case NetworkPriority::High:
                rtcPriority = webrtc::Priority::kHigh;
                break;
            default:
                assert(false);
                break;
        }
        for (auto& encoding : encodings) {
            if (encoding.network_priority != rtcPriority) {
                encoding.network_priority = rtcPriority;
                changed = true;
            }
        }
    }
    return changed;
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::setNetworkPriority(NetworkPriority priority,
                                                          webrtc::RtpParameters& parameters)
{
    return setNetworkPriority(priority, parameters.encodings);
}


} // namespace LiveKitCpp
