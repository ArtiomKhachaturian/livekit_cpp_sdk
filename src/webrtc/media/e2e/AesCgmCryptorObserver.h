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
#pragma once // AesCgmCryptorObserver.h
#include "AesCgmCryptorState.h"
#include <string>

namespace LiveKitCpp
{

class AesCgmCryptorObserver
{
public:
    virtual void onEncryptionStateChanged(const std::string& /*identity*/,
                                          const std::string& /*trackId*/,
                                          AesCgmCryptorState /*state*/) {}
    virtual void onDecryptionStateChanged(const std::string& /*identity*/,
                                          const std::string& /*trackId*/,
                                          AesCgmCryptorState /*state*/) {}
protected:
    virtual ~AesCgmCryptorObserver() = default;
};

} // namespace LiveKitCpp
