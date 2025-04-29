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
#ifndef _WIN32
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#endif
#include <codecvt>
#include <locale>

namespace {

constexpr uint16_t g_crcTable[16] = {
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

}

using ConvertType = std::codecvt_utf8<wchar_t>;

namespace LiveKitCpp
{

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string fromWideChar(const std::wstring& w)
{
    if (!w.empty()) {
        std::wstring_convert<ConvertType, wchar_t> converter;
        return converter.to_bytes(w);
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
