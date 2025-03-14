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
#pragma once // AbslAliasing.h
// https://github.com/abseil/abseil-cpp/issues/264
#include <absl/base/config.h>
#ifdef _MSC_VER
// force to ignore of inconsistent dll linkage
#pragma warning(disable : 4273)
#endif
#ifndef ABSL_USES_STD_STRING_VIEW
#define ABSL_USES_STD_STRING_VIEW
#endif
#ifndef ABSL_USES_STD_ANY
#define ABSL_USES_STD_ANY
#endif
#ifndef ABSL_USES_STD_OPTIONAL
#define ABSL_USES_STD_OPTIONAL
#endif
#ifndef ABSL_USES_STD_VARIANT
#define ABSL_USES_STD_VARIANT
#endif
#include <absl/strings/string_view.h>
#include <absl/types/any.h>
#include <absl/types/optional.h>
#include <absl/types/variant.h>
