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
#include <combaseapi.h>

namespace LiveKitCpp 
{

// Simple scoped memory releaser class for COM allocated memory.
template <typename T>
class ScopedCoMem
{
public:
    ScopedCoMem(T* ptr = nullptr)
        : _ptr(ptr)
    {
    }
    ScopedCoMem(const ScopedCoMem&) = delete;
    ScopedCoMem(ScopedCoMem&& tmp) noexcept
        : _ptr(tmp.take())
    {
    }
    ~ScopedCoMem() { reset(nullptr); }
    static ScopedCoMem alloc(bool zeroed = true)
    {
        const auto ptr = reinterpret_cast<T*>(::CoTaskMemAlloc(sizeof(T)));
        if (ptr && zeroed) {
            ZeroMemory(ptr, sizeof(T));
        }
        return ScopedCoMem(ptr);
    }
    ScopedCoMem& operator=(const ScopedCoMem&) = delete;
    ScopedCoMem& operator=(ScopedCoMem&& tmp)
    {
        if (&tmp != this) {
            reset(tmp.take());
        }
        return *this;
    }
    T** operator&() noexcept { return &_ptr; }
    operator T*() noexcept { return _ptr; }
    T* operator->() noexcept { return _ptr; }
    const T* operator->() const noexcept { return _ptr; }
    explicit operator bool() const noexcept { return nullptr != _ptr; }
    friend bool operator==(const ScopedCoMem& lhs, std::nullptr_t) { return lhs.get() == nullptr; }
    friend bool operator==(std::nullptr_t, const ScopedCoMem& rhs) { return rhs.get() == nullptr; }
    friend bool operator!=(const ScopedCoMem& lhs, std::nullptr_t) { return lhs.get() != nullptr; }
    friend bool operator!=(std::nullptr_t, const ScopedCoMem& rhs) { return rhs.get() != nullptr; }
    void reset(T* ptr)
    {
        if (_ptr) {
            ::CoTaskMemFree(_ptr);
        }
        _ptr = ptr;
    }
    T* get() const noexcept { return _ptr; }
    T* take()
    {
        if (_ptr) {
            T* ptr = nullptr;
            std::swap(ptr, _ptr);
            return ptr;
        }
        return nullptr;
    }
private:
    T* _ptr;
};

} // namespace LiveKitCpp