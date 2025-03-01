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
#pragma once
#include <string>

namespace LiveKitCpp
{

struct Url
{
    Url(std::string host = std::string(),
        std::string user = std::string(),
        std::string password = std::string());
    bool IsValid() const noexcept { return !_host.empty(); }
    explicit operator bool () const noexcept { return IsValid(); }
    // full host info, including scheme & port, for example:
    // 'wss://20.218.159.203:8080/record'
    std::string _host;
    std::string _user;
    std::string _password;
};

inline Url::Url(std::string host, std::string user, std::string password)
    : _host(std::move(host))
    , _user(std::move(user))
    , _password(std::move(password))
{
}

inline bool operator == (const Url& lhs, const Url& rhs) {
    return lhs._host == rhs._host &&
           lhs._user == rhs._user &&
           lhs._password == rhs._password;
}

inline bool operator != (const Url& lhs, const Url& rhs) {
    return lhs._host != rhs._host ||
           lhs._user != rhs._user ||
           lhs._password != rhs._password;
}

} // namespace LiveKitCpp
