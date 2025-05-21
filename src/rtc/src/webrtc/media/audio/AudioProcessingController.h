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
#pragma once // AudioProcessingController.h
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class AudioProcessingController
{
    using Flag = std::atomic_bool;
public:
    AudioProcessingController();
    AudioProcessingController(const AudioProcessingController&) = default;
    AudioProcessingController(AudioProcessingController&&) noexcept = default;
    void setEnableRecProcessing(bool enable);
    void setEnablePlayProcessing(bool enable);
    bool recProcessingEnabled() const;
    bool playProcessingEnabled() const;
    bool processingEnabled() const { return recProcessingEnabled() || playProcessingEnabled(); }
    AudioProcessingController& operator = (const AudioProcessingController&) = default;
    AudioProcessingController& operator = (AudioProcessingController&&) noexcept = default;
private:
    std::shared_ptr<Flag> _enableRecProcessing;
    std::shared_ptr<Flag> _enablePlayProcessing;
};

} // namespace LiveKitCpp
