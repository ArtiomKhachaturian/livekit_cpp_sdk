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
#pragma once // MFD3D9DeviceManager.h
#include "CompletionStatusOr.h"
#include <atlbase.h> //CComPtr support
#include <d3d9.h>
#include <dxva2api.h>

namespace LiveKitCpp 
{

class MFD3D9DeviceManager
{
public:
    static CompletionStatusOr<MFD3D9DeviceManager> create();
    MFD3D9DeviceManager() = default;
    const auto& deviceManager() const noexcept { return _deviceManager; }
private:
    MFD3D9DeviceManager(CComPtr<IDirect3D9Ex> d3d9,
                        CComPtr<IDirect3DDeviceManager9> deviceManager,
                        CComPtr<IDirect3DDevice9Ex> device);
private:
    CComPtr<IDirect3D9Ex> _d3d9;
    CComPtr<IDirect3DDeviceManager9> _deviceManager;
    CComPtr<IDirect3DDevice9Ex> _device;
};

} // namespace LiveKitCpp