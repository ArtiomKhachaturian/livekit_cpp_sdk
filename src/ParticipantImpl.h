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
#pragma once // ParticipantImpl.h
#ifdef WEBRTC_AVAILABLE
#include "Listeners.h"
#include "Participant.h"
#include "ParticipantListener.h"
#include "SafeObj.h"
#include "rtc/ParticipantInfo.h"
#include <type_traits>

namespace LiveKitCpp
{

template<class TListener, class TParticipant = Participant>
class ParticipantImpl : public TParticipant
{
    static_assert(std::is_base_of_v<Participant, TParticipant>);
    static_assert(std::is_base_of_v<ParticipantListener, TListener>);
public:
    virtual void setInfo(const ParticipantInfo& info = {});
    ParticipantInfo info() const { return _info(); }
    void addListener(TListener* listener) final { _listeners.add(listener); }
    void removeListener(TListener* listener) final { _listeners.remove(listener); }
    // impl. of Participant
    std::string sid() const override;
    std::string identity() const override;
    std::string name() const override;
    std::string metadata() const override;
    ParticipantState state() const override;
    bool hasActivePublisher() const override;
    ParticipantKind kind() const override;
protected:
    Bricks::SafeObj<ParticipantInfo> _info;
    Bricks::Listeners<TListener*> _listeners;
};

template<class TListener, class TParticipant>
inline void ParticipantImpl<TListener, TParticipant>::setInfo(const ParticipantInfo& info)
{
    LOCK_WRITE_SAFE_OBJ(_info);
    _info = info;
    _listeners.invoke(&ParticipantListener::onChanged, this);
}

template<class TListener, class TParticipant>
inline std::string ParticipantImpl<TListener, TParticipant>::sid() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_sid;
}

template<class TListener, class TParticipant>
inline std::string ParticipantImpl<TListener, TParticipant>::identity() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_identity;
}

template<class TListener, class TParticipant>
inline std::string ParticipantImpl<TListener, TParticipant>::name() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_name;
}

template<class TListener, class TParticipant>
inline std::string ParticipantImpl<TListener, TParticipant>::metadata() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_metadata;
}

template<class TListener, class TParticipant>
inline ParticipantState ParticipantImpl<TListener, TParticipant>::state() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_state;
}

template<class TListener, class TParticipant>
inline bool ParticipantImpl<TListener, TParticipant>::hasActivePublisher() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_isPublisher;
}

template<class TListener, class TParticipant>
inline ParticipantKind ParticipantImpl<TListener, TParticipant>::kind() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_kind;
}

} // namespace LiveKitCpp
#endif
