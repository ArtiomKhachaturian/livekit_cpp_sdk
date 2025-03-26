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
#pragma once // E2ESecurityFactory.h
#include "TrackManager.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <string>

namespace LiveKitCpp
{

class AesCgmCryptor;

class E2ESecurityFactory : public TrackManager
{
public:
    virtual webrtc::scoped_refptr<AesCgmCryptor> createCryptor(bool local,
                                                               cricket::MediaType mediaType,
                                                               std::string id) const = 0;
protected:
    ~E2ESecurityFactory() override = default;
};

} // namespace LiveKitCpp
