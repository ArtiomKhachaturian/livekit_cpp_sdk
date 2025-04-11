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
#pragma once // MarshalledTypesFwd.h
#include <string>

#define MARSHALLED_TYPE_NAME_DECL(name) \
    template <> std::string marshalledTypeName<name>() { return #name; }

namespace LiveKitCpp
{

template <typename T>
inline std::string marshalledTypeName() {
    static_assert(false, "type name not evaluated, "
                         "use MARSHALLED_TYPE_NAME_DECL macro "
                         "for the type name declaration");
}

} // namespace LiveKitCpp
