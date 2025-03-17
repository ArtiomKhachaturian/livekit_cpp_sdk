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
#include "Participant.h"
#include "SafeObj.h"
#include "rtc/ParticipantInfo.h"
#include <type_traits>

namespace LiveKitCpp
{

template<class TParticipant = Participant>
class ParticipantImpl : public TParticipant
{
    static_assert(std::is_base_of_v<Participant, TParticipant>);
public:
    virtual void setInfo(const ParticipantInfo& info = {});
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
};

template<class TParticipant>
inline void ParticipantImpl<TParticipant>::setInfo(const ParticipantInfo& info)
{
    LOCK_WRITE_SAFE_OBJ(_info);
    _info = info;
}

template<class TParticipant>
inline std::string ParticipantImpl<TParticipant>::sid() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_sid;
}

template<class TParticipant>
inline std::string ParticipantImpl<TParticipant>::identity() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_identity;
}

template<class TParticipant>
inline std::string ParticipantImpl<TParticipant>::name() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_name;
}

template<class TParticipant>
inline std::string ParticipantImpl<TParticipant>::metadata() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_metadata;
}

template<class TParticipant>
inline ParticipantState ParticipantImpl<TParticipant>::state() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_state;
}

template<class TParticipant>
inline bool ParticipantImpl<TParticipant>::hasActivePublisher() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_isPublisher;
}

template<class TParticipant>
inline ParticipantKind ParticipantImpl<TParticipant>::kind() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_kind;
}

} // namespace LiveKitCpp
#endif
