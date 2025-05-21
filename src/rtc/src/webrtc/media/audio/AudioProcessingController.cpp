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
#include "AudioProcessingController.h"

namespace LiveKitCpp
{

AudioProcessingController::AudioProcessingController()
    : _enableRecProcessing(std::make_shared<Flag>(true))
    , _enablePlayProcessing(std::make_shared<Flag>(true))
{
}

void AudioProcessingController::setEnableRecProcessing(bool enable)
{
    if (_enableRecProcessing) {
        _enableRecProcessing->store(enable);
    }
}

void AudioProcessingController::setEnablePlayProcessing(bool enable)
{
    if (_enablePlayProcessing) {
        _enablePlayProcessing->store(enable);
    }
}

bool AudioProcessingController::recProcessingEnabled() const
{
    return _enableRecProcessing && _enableRecProcessing->load();
}

bool AudioProcessingController::playProcessingEnabled() const
{
    return _enablePlayProcessing && _enablePlayProcessing->load();
}

} // namespace LiveKitCpp
