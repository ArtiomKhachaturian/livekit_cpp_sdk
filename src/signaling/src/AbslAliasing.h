/********************************************************************************************************************
 **                                                                                                               **
 ** Copyright (C) 2020, Dark Matter LLC. All rights Reserved                                                      **
 ** This software and/or source code may be used, copied and/or disseminated only with the written                **
 ** permission of Dark Matter LLC, or in accordance with the terms and conditions stipulated in the               **
 ** agreement/contract under which the software and/or source code has been                                       **
 ** supplied by Dark Matter LLC or its affiliates. Unauthorized use, copying, or dissemination of this file, via  **
 ** any medium, is strictly prohibited, and will constitute an infringement of copyright.                         **
 **                                                                                                               **
*********************************************************************************************************************/
#pragma once
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
