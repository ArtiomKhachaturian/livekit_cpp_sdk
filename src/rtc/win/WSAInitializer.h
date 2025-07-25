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
#pragma once // WSAInitializer.h
#include <optional>
#include <string>
#include <rtc_base/win32.h>

namespace LiveKitCpp
{

class WSAInitializer
{
public:
    enum class Version : WORD
    {
        v1_0 = MAKEWORD(1, 0),
        v1_1 = MAKEWORD(1, 1),
        v2_0 = MAKEWORD(2, 0),
        v2_1 = MAKEWORD(2, 1),
        v2_2 = MAKEWORD(2, 2)
    };
public:
    WSAInitializer();
    ~WSAInitializer();
    int GetError() const noexcept { return _error; }
    const auto& GetSelectedVersion() const noexcept { return _selectedVersion; }
    static std::string ToString(Version version);
private:
    static int WsaStartup(Version version, WSADATA& wsaData);
private:
    int _error = 0;
    std::optional<Version> _selectedVersion;
};

} // namespace LiveKitCpp
