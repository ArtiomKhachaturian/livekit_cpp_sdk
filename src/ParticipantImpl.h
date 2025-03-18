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
#include <type_traits>

namespace LiveKitCpp
{

struct ParticipantInfo;

template<class TListener, class TParticipant = Participant>
class ParticipantImpl : public TParticipant
{
    static_assert(std::is_base_of_v<Participant, TParticipant>);
    static_assert(std::is_base_of_v<ParticipantListener, TListener>);
public:
    virtual void setInfo(const ParticipantInfo& info) = 0;
    void addListener(TListener* listener) final { _listeners.add(listener); }
    void removeListener(TListener* listener) final { _listeners.remove(listener); }
protected:
    void fireOnChanged() { _listeners.invoke(&ParticipantListener::onChanged, this); }
protected:
    Bricks::Listeners<TListener*> _listeners;
};

} // namespace LiveKitCpp
#endif
