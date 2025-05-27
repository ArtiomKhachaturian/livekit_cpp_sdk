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
#pragma once // CompletionStatusOr.h
#include "CompletionStatus.h"
#ifdef WEBRTC_WIN
#include <atlbase.h> //CComPtr support
#endif
#include <api/scoped_refptr.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

// common processing of operation status result
template <typename T>
class CompletionStatusOr
{
    static_assert(!std::is_same_v<T, CompletionStatus>, "value type should not be CompletionStatus");
    // used to convert between CompletionStatusOr<Foo> / CompletionStatusOr<Bar>,
    // when an implicit conversion from Foo to Bar exists
    template <typename U>
    friend class CompletionStatusOr;
public:
    CompletionStatusOr(const CompletionStatusOr&) = delete;
    CompletionStatusOr(CompletionStatusOr&& other) noexcept;
    CompletionStatusOr(const CompletionStatus& status);
    CompletionStatusOr(CompletionStatus&& status);
    CompletionStatusOr(const T& value);
    CompletionStatusOr(T&& value) noexcept;
    template <typename U>
    CompletionStatusOr(CompletionStatusOr<U> other) noexcept;
    CompletionStatusOr& operator=(const CompletionStatusOr&) = delete;
    CompletionStatusOr& operator=(CompletionStatusOr&& other) noexcept;
    template <typename U>
    CompletionStatusOr& operator=(CompletionStatusOr<U> other) noexcept;
    auto code() const noexcept { return status().code(); }
    const auto& function() const noexcept { return status().function(); }
    auto what() const { return status().what(); }
    bool ok() const noexcept { return status().ok(); }
    explicit operator bool() const noexcept { return ok(); }
    const auto& status() const noexcept { return _status; }
    auto moveStatus() noexcept { return std::move(_status); }
    const T& value() const noexcept { return _value; }
    T& value() noexcept { return _value; }
    T moveValue() noexcept { return std::move(_value); }
    const T* operator->() const noexcept { return &_value; }
    T* operator->() noexcept { return &_value; }
    template <class... Types>
    static CompletionStatusOr emplace(Types&&... args);
private:
    CompletionStatus _status;
    T _value;
};

template <typename T>
inline CompletionStatusOr<T>::CompletionStatusOr(CompletionStatusOr&& other) noexcept
    : _status(std::move(other._status))
    , _value(std::move(other._value))
{
}

template <typename T>
inline CompletionStatusOr<T>::CompletionStatusOr(const CompletionStatus& status)
    : _status(status)
{
}

template <typename T>
inline CompletionStatusOr<T>::CompletionStatusOr(CompletionStatus&& status)
    : _status(std::move(status))
{
}

template <typename T>
inline CompletionStatusOr<T>::CompletionStatusOr(const T& value)
    : _value(value)
{
}

template <typename T>
inline CompletionStatusOr<T>::CompletionStatusOr(T&& value) noexcept
    : _value(std::move(value))
{
}

template <typename T>
template <typename U>
inline CompletionStatusOr<T>::CompletionStatusOr(CompletionStatusOr<U> other) noexcept
    : _status(std::move(other._status))
    , _value(std::move(other._value))
{
}

template <typename T>
inline CompletionStatusOr<T>& CompletionStatusOr<T>::operator=(CompletionStatusOr&& other) noexcept
{
    if (this != &other) {
        _status = std::move(other._status);
        _value = std::move(other._value);
    }
    return *this;
}

template <typename T>
template <typename U>
inline CompletionStatusOr<T>& CompletionStatusOr<T>::operator=(CompletionStatusOr<U> other) noexcept
{
    _status = std::move(other._status);
    _value = std::move(other._value);
    return *this;
}

template <typename T>
template <class... Types>
inline CompletionStatusOr<T> CompletionStatusOr<T>::emplace(Types&&... args)
{
    return CompletionStatusOr<T>(T(std::forward<Types>(args)...));
}

// smart pointer typedefs
template <class T>
using CompletionStatusOrUniquePtr = CompletionStatusOr<std::unique_ptr<T>>;
template <class T>
using CompletionStatusOrSharedPtr = CompletionStatusOr<std::shared_ptr<T>>;
template <class T>
using CompletionStatusOrScopedRefPtr = CompletionStatusOr<webrtc::scoped_refptr<T>>;
#ifdef WEBRTC_WIN
template <class TComInterface>
using CompletionStatusOrComPtr = CompletionStatusOr<CComPtr<TComInterface>>;
#endif

} // namespace LiveKitCpp


template <typename T, typename U = T>
inline bool operator == (const LiveKitCpp::CompletionStatusOr<T>& l,
                         const LiveKitCpp::CompletionStatusOr<U>& r)
{
    return l.value() == r.value() && l.errorInfo() == r.errorInfo();
}

template <typename T, typename U = T>
inline bool operator != (const LiveKitCpp::CompletionStatusOr<T>& l,
                         const LiveKitCpp::CompletionStatusOr<U>& r)
{
    return l.value() != r.value() || l.errorInfo() != r.errorInfo();
}
