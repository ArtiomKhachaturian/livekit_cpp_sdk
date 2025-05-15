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
#include "Utils.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#endif
#include <stdlib.h>

namespace {

constexpr uint16_t g_crcTable[16] = {
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

#ifdef _WIN32
class ScopedComInitializer : public LiveKitCpp::ComStatus
{
public:
    ScopedComInitializer(DWORD coInit);
    ~ScopedComInitializer() final;
private:
    const bool _differentApartment;
};
#endif

}

namespace LiveKitCpp
{

#ifdef _WIN32
std::tuple<int, int, int> operatingSystemVersion()
{
    RTL_OSVERSIONINFOEXW osv = {0};
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
        // RtlGetVersion is documented public API but we must load it dynamically
        // because linking to it at load time will not pass the Windows App Certification Kit
        // https://msdn.microsoft.com/en-us/library/windows/hardware/ff561910.aspx
        if (auto RtlGetVersion = (RtlGetVersionPtr)::GetProcAddress(ntdll, "RtlGetVersion")) {
            osv.dwOSVersionInfoSize = sizeof(osv);
            // GetVersionEx() has been deprecated in Windows 8.1 and will return
            // only Windows 8 from that version on, so use the kernel API function.
            RtlGetVersion(&osv); // always returns STATUS_SUCCESS
        }
    }
    return std::make_tuple(osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
}

ComStatus initializeComForThisThread(DWORD coInit)
{
    static thread_local const ScopedComInitializer init(coInit);
    return init;
}
#endif

std::string operatingSystemVersionString(bool withPatchVersion)
{
    const auto osv = operatingSystemVersion();
    std::vector<std::string> components;
    components.push_back(std::to_string(std::get<0>(osv)));
    components.push_back(std::to_string(std::get<1>(osv)));
    if (withPatchVersion) {
        components.push_back(std::to_string(std::get<2>(osv)));
    }
    return join(components, ".");
}

std::string operatingSystemName()
{
#ifdef __APPLE__
    return "macos";
#elif defined(_WIN32)
    return "windows";
#else
    static_assert(false, "not yet implemented");
#endif
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string fromWideChar(std::wstring_view w)
{
    if (size_t size = w.size()) {
        std::string out;
        out.resize(size);
#ifdef __APPLE__
        if (size == wcstombs(out.data(), w.data(), w.size())) {
            return out;
        }
#else
        if (0 == wcstombs_s(&size, out.data(), out.size() + 1U, w.data(), w.size())) {
            return out;
        }
#endif
    }
    return {};
}

bool compareCaseInsensitive(std::string_view s1, std::string_view s2)
{
    const size_t size = s1.size();
    if (size == s2.size()) {
        for (size_t i = 0U; i < size; ++i) {
            if (std::tolower(s1[i]) != std::tolower(s2[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

std::vector<std::string> split(std::string_view s, std::string_view delim)
{
    std::vector<std::string> res;
    if (!s.empty() && !delim.empty()) {
        std::string::size_type pos, lastPos = 0U, length = s.length();
        while (lastPos < length + 1U) {
            pos = s.find_first_of(delim, lastPos);
            if (pos == std::string::npos) {
                pos = length;
            }
            if (pos != lastPos)
                res.emplace_back(s.data() + lastPos, pos - lastPos);

            lastPos = pos + 1U;
        }
    }
    return res;
}

std::string join(const std::vector<std::string>& strings,
                 std::string_view delim, bool skipEmpty)
{
    if (!strings.empty()) {
        const std::vector<std::string_view> stringViews(strings.begin(), strings.end());
        return join(stringViews, delim, skipEmpty);
    }
    return {};
}

std::string join(const std::vector<std::string_view>& strings,
                 std::string_view delim, bool skipEmpty)
{
    if (!strings.empty()) {
        const auto size = strings.size();
        if (size > 1U) {
            std::string result;
            for (size_t i = 0U, last = size - 1U; i < size; ++i) {
                const auto& string = strings.at(i);
                if (!skipEmpty || !string.empty()) {
                    result += string;
                    if (i != last) {
                        result += delim;
                    }
                }
            }
            return result;
        }
        return std::string(strings.front().data(), strings.front().size());
    }
    return {};
}

uint16_t checksumISO3309(const uint8_t* data, size_t len)
{
    if (data && len) {
        uint16_t crc = 0xffff;
        uint8_t c;
        while (len--) {
            c = *data++;
            crc = ((crc >> 4) & 0x0fff) ^ g_crcTable[((crc ^ c) & 15)];
            c >>= 4;
            crc = ((crc >> 4) & 0x0fff) ^ g_crcTable[((crc ^ c) & 15)];
        }
        crc = ~crc;
        return crc & 0xffff;
    }
    return 0U;
}

} // namespace LiveKitCpp

#ifdef _WIN32
namespace
{

ScopedComInitializer::ScopedComInitializer(DWORD coInit)
    : LiveKitCpp::ComStatus(::CoInitializeEx(NULL, coInit))
    , _differentApartment(RPC_E_CHANGED_MODE == status())
{
    if (_differentApartment) {
        setStatus(S_OK);
    }
}

ScopedComInitializer::~ScopedComInitializer()
{
    if (ok() && !_differentApartment) {
        ::CoUninitialize();
    }
}

}
#endif