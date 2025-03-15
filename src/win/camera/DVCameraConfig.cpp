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
#include "DVCameraConfig.h"
#include "CameraErrorHandling.h"
#include <cassert>

namespace LiveKitCpp
{

WinCameraCapturer::DVCameraConfig::DVCameraConfig(const CComPtr<IGraphBuilder>& graphBuilder,
                                                  const CComPtr<IPin>& inputDvPin,
                                                  const CComPtr<IPin>& outputDvPin,
                                                  const CComPtr<IBaseFilter>& dvFilter,
                                                  const CComPtr<IPin>& inputSendPin,
                                                  const CComPtr<IPin>& outputCapturePin,
                                                  const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _graphBuilder(graphBuilder)
    , _inputDvPin(inputDvPin)
    , _outputDvPin(outputDvPin)
    , _dvFilter(dvFilter)
    , _inputSendPin(inputSendPin)
    , _outputCapturePin(outputCapturePin)
{
    assert(_graphBuilder);
    assert(_inputDvPin);
    assert(_outputDvPin);
    assert(_dvFilter);
    assert(_inputSendPin);
    assert(_outputCapturePin);
}

WinCameraCapturer::DVCameraConfig::~DVCameraConfig()
{
    _graphBuilder->RemoveFilter(_dvFilter);
}

bool WinCameraCapturer::DVCameraConfig::connect()
{
    if (!LOGGABLE_COM_IS_OK(_graphBuilder->ConnectDirect(_outputCapturePin, _inputDvPin, NULL))) {
        return false;
    }
    return LOGGABLE_COM_IS_OK(_graphBuilder->ConnectDirect(_outputDvPin, _inputSendPin, NULL));
}

void WinCameraCapturer::DVCameraConfig::disconnect()
{
    LOGGABLE_COM_ERROR(_graphBuilder->Disconnect(_inputDvPin));
    LOGGABLE_COM_ERROR(_graphBuilder->Disconnect(_outputDvPin));
}

} // namespace LiveKitCpp
