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
#pragma once // CommandReceiver.h
#include <string>

namespace LiveKitCpp
{

class CommandReceiver
{
public:
    virtual ~CommandReceiver() = default;
    virtual void receiveBinary(const void* data, size_t dataLen) = 0;
    virtual void receiveText(const std::string_view& text) = 0;
};

} // namespace LiveKitCpp
