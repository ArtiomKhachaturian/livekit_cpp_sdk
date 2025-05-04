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
#pragma once // VTSessionPipeline.h
#include "VideoUtils.h"
#include "Utils.h"
#include <CoreMedia/CMFormatDescription.h>
#include <atomic>

namespace LiveKitCpp
{

class VTEncoderSessionCallback;

template <class TPipelineCallback>
inline bool isEncoderPipeline() { return false; }
template <>
inline bool isEncoderPipeline<VTEncoderSessionCallback>() { return true; }

template <class TPipelineCallback>
class VTSessionPipeline
{
public:
    OSStatus lastOutputStatus() const { return _lastOutputStatus.load(std::memory_order_relaxed); }
    uint64_t inputFramesCount() const { return _inputFramesCount.load(std::memory_order_relaxed); }
    uint64_t outputFramesCount() const { return _outputFramesCount.load(std::memory_order_relaxed); }
    uint64_t outputErrorsCount() const { return _outputErrorsCount.load(std::memory_order_relaxed); }
    uint64_t pendingFramesCount() const { return inputFramesCount() - outputFramesCount(); }
    bool active() const { return _active.load(std::memory_order_relaxed); }
    void setActive(bool active = true) { _active = active; }
protected:
    VTSessionPipeline(CMVideoCodecType codecType, TPipelineCallback* callback = nullptr);
    ~VTSessionPipeline();
    void beginInput() { _inputFramesCount.fetch_add(1ULL); }
    OSStatus endInput(OSStatus result);
    bool endOutput(OSStatus result);
    template <class Method, typename... Args>
    void callback(const Method& method, Args&&... args) const;
private:
    std::string name() const { return videoCodecTypeToString(_codecType); }
private:
    static inline const bool _isEncoderPipeline = isEncoderPipeline<TPipelineCallback>();
    const CMVideoCodecType _codecType;
    TPipelineCallback* const _callback;
    std::atomic<OSStatus> _lastOutputStatus = noErr;
    std::atomic<uint64_t> _inputFramesCount = 0ULL;
    std::atomic<uint64_t> _outputFramesCount = 0ULL;
    std::atomic<uint64_t> _outputErrorsCount = 0ULL;
    std::atomic_bool _active = true;
};

template <class TPipelineCallback>
VTSessionPipeline<TPipelineCallback>::VTSessionPipeline(CMVideoCodecType codecType,
                                                        TPipelineCallback* callback)
    :  _codecType(codecType)
    , _callback(callback)
{
}

template <class TPipelineCallback>
VTSessionPipeline<TPipelineCallback>::~VTSessionPipeline()
{
    if (const auto pendingFrames = pendingFramesCount()) {
        RTC_LOG(LS_INFO) << name() << ": non-" << (_isEncoderPipeline ? "encoded" : "decoded")
                         << " frames count is " << pendingFrames;
    }
    if (const auto errors = outputErrorsCount()) {
        const auto errorRatio = static_cast<uint32_t>(std::round(errors / (outputFramesCount() / 100.)));
        RTC_LOG(LS_WARNING) << name() << ": " << (_isEncoderPipeline ? "encode" : "decode")
                            << " errors " << errorRatio << "%";
    }
}

template <class TPipelineCallback>
OSStatus VTSessionPipeline<TPipelineCallback>::endInput(OSStatus result)
{
    if (noErr != result) {
        _inputFramesCount.fetch_sub(1ULL);
    }
    return result;
}

template <class TPipelineCallback>
bool VTSessionPipeline<TPipelineCallback>::endOutput(OSStatus result)
{
    _outputFramesCount.fetch_add(1ULL);
    if (noErr != result) {
        _outputErrorsCount.fetch_add(1ULL);
    }
    return exchangeVal(result, _lastOutputStatus);
}

template <class TPipelineCallback>
template <class Method, typename... Args>
void VTSessionPipeline<TPipelineCallback>::callback(const Method& method, Args&&... args) const
{
    if (_callback && active()) {
        (_callback->*method)(std::forward<Args>(args)...);
    }
}

} // namespace LiveKitCpp
