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
#pragma once // DVCameraConfig.h
#include "Loggable.h"
#include <atlbase.h> //CComPtr support
#include <Dshow.h>

namespace LiveKitCpp
{

// Microsoft DV interface (external DV cameras)
class DVCameraConfig : public Bricks::LoggableS<>
{
public:
    DVCameraConfig(const CComPtr<IGraphBuilder>& graphBuilder,
                   const CComPtr<IPin>& inputDvPin,
                   const CComPtr<IPin>& outputDvPin,
                   const CComPtr<IBaseFilter>& dvFilter,
                   const CComPtr<IPin>& inputSendPin,
                   const CComPtr<IPin>& outputCapturePin,
                   const std::shared_ptr<Bricks::Logger>& logger);
    ~DVCameraConfig();
    bool connect();
    void disconnect();
protected:
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final;
private:
    const CComPtr<IGraphBuilder> _graphBuilder;
    const CComPtr<IPin> _inputDvPin;
    const CComPtr<IPin> _outputDvPin;
    const CComPtr<IBaseFilter> _dvFilter;
    const CComPtr<IPin> _inputSendPin;
    const CComPtr<IPin> _outputCapturePin;
};

} // namespace LiveKitCpp