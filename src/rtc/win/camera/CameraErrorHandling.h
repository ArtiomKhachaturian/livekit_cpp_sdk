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
#pragma once // CameraErrorHandling.h
#include "ComErrorHandling.h"
#include "CameraManager.h"

#define LOG_CAMERA_ERROR(hr, logger) LOG_COM_ERROR(hr, logger, LiveKitCpp::CameraManager::logCategory())
#define CAMERA_IS_OK(hr, logger) COM_IS_OK(hr, logger, LiveKitCpp::CameraManager::logCategory())