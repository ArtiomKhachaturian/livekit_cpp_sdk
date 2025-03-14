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
#pragma once // LocalVideoTrack.h
#include "LocalTrackImpl.h"
#include "VideoTrack.h"

namespace LiveKitCpp
{

template<class TMediaTrack = webrtc::VideoTrackInterface>
class LocalVideoTrack : public LocalTrackImpl<TMediaTrack, VideoTrack>
{
    using Base = LocalTrackImpl<TMediaTrack, VideoTrack>;
protected:
    LocalVideoTrack(std::string name, LocalTrackManager* manager,
                    const std::shared_ptr<Bricks::Logger>& logger = {});
};

template<class TMediaTrack>
inline LocalVideoTrack<TMediaTrack>::LocalVideoTrack(std::string name,
                                                     LocalTrackManager* manager,
                                                     const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(name), manager, logger)
{
}

} // namespace LiveKitCpp
