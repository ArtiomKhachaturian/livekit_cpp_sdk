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
#pragma once // ComStatus.h
#ifdef _WIN32
#include <comdef.h>

namespace LiveKitCpp 
{

class ComStatus
{
public:
    virtual ~ComStatus() = default;
    ComStatus& operator=(const ComStatus& other) noexcept;
    ComStatus& operator=(ComStatus&& tmp) noexcept;
    operator HRESULT() const noexcept { return status(); }
    bool ok() const noexcept { return SUCCEEDED(status()); }
    HRESULT status() const noexcept { return _status; }
    explicit operator bool() const noexcept { return ok(); }
protected:
    ComStatus() = default;
    explicit ComStatus(HRESULT status) noexcept;
    ComStatus(const ComStatus& other) noexcept;
    ComStatus(ComStatus&& tmp) noexcept;
    void assign(ComStatus&& tmp) noexcept;
    void setStatus(HRESULT status) noexcept;
private:
    HRESULT _status = E_INVALIDARG;
};

} // namespace LiveKitCpp
#endif