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
#ifdef _WIN32
#include "ComStatus.h"
#include <utility>

namespace LiveKitCpp 
{

ComStatus::ComStatus(HRESULT status) noexcept
    : _status(status)
{
}

ComStatus::ComStatus(ComStatus&& tmp) noexcept
{
    assign(std::move(tmp));
}

ComStatus::ComStatus(const ComStatus& other) noexcept
{
    setStatus(other.status());
}

ComStatus& ComStatus::operator=(ComStatus&& tmp) noexcept
{
    assign(std::move(tmp));
    return *this;
}

ComStatus& ComStatus::operator=(const ComStatus& other) noexcept
{
    if (&other != this) {
        setStatus(other.status());
    }
    return *this;
}

void ComStatus::assign(ComStatus&& tmp) noexcept
{
    if (this != &tmp) {
        std::swap(_status, tmp._status);
    }
}

void ComStatus::setStatus(HRESULT status) noexcept
{
    _status = status;
}

} // namespace LiveKitCpp
#endif