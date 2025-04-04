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
#include "stats/StatsAttribute.h"
#include "Utils.h"

namespace
{
struct VisitIsSequence
{
  // Any type of vector is a sequence.
    template <typename T>
    bool operator()(const std::optional<std::vector<T>>*) const { return true; }
    // Any other type is not.
    template <typename T>
    bool operator()(const std::optional<T>*) const { return false; }
};

struct VisitIsMap
{
  // Any type of vector is a sequence.
    template <typename T>
    bool operator()(const std::optional<std::map<std::string, T>>*) const { return true; }
    // Any other type is not.
    template<typename T>
    bool operator()(const std::optional<T>*) const { return false; }
};

struct VisitToString
{
    template<typename T>
    std::string operator()(const std::optional<T>* attr) const;
private:
    static const std::string& toString(const std::string& s) { return s; }
    static std::string toString(bool v) { return v ? "true" : "false"; }
    static std::string toString(int32_t v) { return std::to_string(v); }
    static std::string toString(uint32_t v) { return std::to_string(v); }
    static std::string toString(int64_t v) { return std::to_string(v); }
    static std::string toString(uint64_t v) { return std::to_string(v); }
    static std::string toString(double v) { return std::to_string(v); }
    template<typename T>
    static std::string toString(const std::vector<T>& v);
    template<typename T>
    static std::string toString(const std::map<std::string, T>& v);
    static std::string join(const std::vector<std::string>& strings);
};

}

namespace LiveKitCpp
{

StatsAttribute::StatsAttribute(std::string_view name, Value value)
    : _name(std::move(name))
    , _value(std::move(value))
{
}

std::string_view StatsAttribute::name() const
{
    return _name;
}

bool StatsAttribute::valid() const
{
    return std::visit([](const auto* attr) { return attr->has_value(); }, _value);
}

bool StatsAttribute::isSequence() const
{
    return std::visit(VisitIsSequence{}, _value);
}

bool StatsAttribute::isAssociative() const
{
    return std::visit(VisitIsMap{}, _value);
}

std::string StatsAttribute::toString() const
{
    if (valid()) {
        return std::visit(VisitToString{}, _value);
    }
    return {};
}

bool StatsAttribute::operator == (const StatsAttribute& other) const
{
    if (&other != this) {
        return _value == other._value;
    }
    return true;
}

bool StatsAttribute::operator != (const StatsAttribute& other) const
{
    return &other != this || _value != other._value;
}

} // namespace LiveKitCpp

namespace
{

template<typename T>
std::string VisitToString::operator()(const std::optional<T>* attr) const
{
    if (attr && attr->has_value()) {
        return toString(attr->value());
    }
    return {};
}

template<typename T>
std::string VisitToString::toString(const std::vector<T>& v)
{
    if (const auto s = v.size()) {
        std::vector<std::string> strings;
        strings.reserve(s);
        for (size_t i = 0U; i < s; ++i) {
            strings.push_back(toString(v.at(i)));
        }
        return join(strings);
    }
    return {};
}

template<typename T>
std::string VisitToString::toString(const std::map<std::string, T>& v)
{
    if (const auto s = v.size()) {
        std::vector<std::string> strings;
        strings.reserve(s);
        for (auto it = v.begin(); it != v.end(); ++it) {
            strings.push_back(it->first + ":" + toString(it->second));
        }
        return join(strings);
    }
    return {};
}

std::string VisitToString::join(const std::vector<std::string>& strings)
{
    return LiveKitCpp::join(strings, ",", false);
}

}
