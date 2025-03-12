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
#include "Track.h"
#include "SafeObj.h"

namespace LiveKitCpp
{

struct AddTrackRequest;

class LocalTrack : public Track
{
public:
    // for publishing
    virtual bool live() const noexcept = 0;
    virtual void fillRequest(AddTrackRequest* request) const = 0;
    // transport layer
    virtual void resetMedia(bool remove = true) = 0;
    virtual void notifyThaMediaAddedToTransport() = 0;
    // client track ID, equal to WebRTC track ID
    const std::string& cid() const noexcept { return _cid; }
    // track name
    const std::string& name() const noexcept { return _name; }
    void setSid(const std::string& sid) { _sid(sid); }
    // impl. of Track
    std::string sid() const final { return _sid(); }
    bool remote() const noexcept final { return false; }
protected:
    LocalTrack(std::string name);
private:
    const std::string _cid;
    const std::string _name;
    Bricks::SafeObj<std::string> _sid;
};

} // namespace LiveKitCpp
