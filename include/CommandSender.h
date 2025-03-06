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
#pragma once // CommandSender.h
#include <memory>
#include <string>

// prototype defined in 'Bricks' library,
// see https://github.com/ArtiomKhachaturian/Bricks
namespace Bricks {
class Blob;
}

namespace LiveKitCpp
{

class CommandSender
{
public:
    virtual bool sendBinary(const std::shared_ptr<Bricks::Blob>& /*binary*/) {
        return false;
    }
    virtual bool sendText(const std::string_view& /*text*/) {
        return false;
    }
protected:
    virtual ~CommandSender() = default;
};

} // namespace LiveKitCpp
