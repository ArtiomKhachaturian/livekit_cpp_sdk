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
#pragma once // ComErrorHandling.h
#ifdef WEBRTC_WIN
#include <memory>
#include <system_error>
#include <Windows.h>
#include <comdef.h>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

std::string comErrorToString(const _com_error& error, const char* function = "", int lineNumber = -1);
std::string comErrorToString(HRESULT hr, const char* function = "", int lineNumber = -1);
std::string comErrorToString(const std::system_error& e, const char* function = "", int lineNumber = -1);
HRESULT logComError(HRESULT hr, const char* function = "", int lineNumber = -1,
                    const std::shared_ptr<Bricks::Logger>& logger = {},
                    std::string_view category = {});

} // namespace LiveKitCpp

#define LOG_COM_ERROR(hr, logger, category) logComError(HRESULT(hr), __FUNCSIG__, __LINE__, logger, category)
#define COM_IS_OK(hr, logger, category) SUCCEEDED(LOG_COM_ERROR(hr, logger, category))
// both these macro are actual only inside of Bricks::Loggable descendants
#define LOGGABLE_COM_ERROR(hr) LOG_COM_ERROR(hr, logger(), logCategory())
#define LOGGABLE_COM_IS_OK(hr) SUCCEEDED(LOGGABLE_COM_ERROR(hr))

namespace std 
{

template<> struct is_error_code_enum<HRESULT> { 
    static inline constexpr bool value = true; 
};

inline std::error_code make_error_code(HRESULT hr) noexcept { 
    return std::error_code(int(hr), std::system_category()); 
}

}
#endif