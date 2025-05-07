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
#include "MFD3D9DeviceManager.h"
#include "RtcUtils.h"

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "D3d9.lib")
#pragma comment(lib, "evr.lib")

namespace LiveKitCpp 
{

MFD3D9DeviceManager::MFD3D9DeviceManager(CComPtr<IDirect3D9Ex> d3d9,
                                         CComPtr<IDirect3DDeviceManager9> deviceManager,
                                         CComPtr<IDirect3DDevice9Ex> device)
    : _d3d9(std::move(d3d9))
    , _deviceManager(std::move(deviceManager))
    , _device(std::move(device))
{
}

CompletionStatusOr<MFD3D9DeviceManager> MFD3D9DeviceManager::create()
{
    CComPtr<IDirect3D9Ex> d3d9;
    auto hr = COMPLETION_STATUS(::Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9));
    if (!hr) {
        return hr;
    }
    CComPtr<IDirect3DDeviceManager9> deviceManager;
    UINT deviceResetToken;
    hr = COMPLETION_STATUS(::DXVA2CreateDirect3DDeviceManager9(&deviceResetToken, &deviceManager));
    if (!hr) {
        return hr;
    }
    CComPtr<IDirect3DDevice9Ex> device;
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = ::GetShellWindow();
    pp.Windowed = TRUE;
    pp.Flags = D3DPRESENTFLAG_VIDEO;
    pp.FullScreen_RefreshRateInHz = 0;
    pp.PresentationInterval = 0;
    hr = COMPLETION_STATUS(d3d9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pp.hDeviceWindow,
                                                D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                                D3DCREATE_DISABLE_PSGP_THREADING | D3DCREATE_MULTITHREADED,
                                                &pp, NULL, &device));
    if (!hr) {
        return hr;
    }
    // reset the D3DDeviceManager with the new device
    hr = COMPLETION_STATUS(deviceManager->ResetDevice(device, deviceResetToken));
    if (!hr) {
        return hr;
    }
    return MFD3D9DeviceManager(d3d9, deviceManager, device);
}

} // namespace LiveKitCpp