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
#include "DesktopCapturerUtils.h"
#include "Utils.h"
#include <modules/desktop_capture/win/screen_capture_utils.h>
#include <modules/desktop_capture/win/window_capture_utils.h>
#ifdef RTC_ENABLE_WIN_WGC
#include <rtc_base/win/hstring.h>
#include <rtc_base/string_utils.h> // required for string manipulations
#include <rtc_base/win/get_activation_factory.h>
#include <windows.graphics.capture.h>
#include <windows.foundation.metadata.h>
#endif

namespace
{
constexpr char g_devPrefix[] = "\\\\.\\";

bool windowIsValidForPreview(HWND hwnd);
HMONITOR hMonitorFromDeviceIndex(webrtc::ScreenId sId);
std::string screenFriendlyTitle(const wchar_t* name);
#ifdef RTC_ENABLE_WIN_WGC
bool testWgcFramework();
#endif

}

namespace LiveKitCpp
{

bool screenExists(webrtc::ScreenId sId)
{
    if (webrtc::kInvalidScreenId != sId) {
        if (webrtc::kFullDesktopScreenId != sId) {
            return NULL != hMonitorFromDeviceIndex(sId);
        }
        return true;
    }
    return false;
}

bool windowExists(webrtc::WindowId wId)
{
    return webrtc::kNullWindowId != wId && NULL != ::IsWindow(reinterpret_cast<HWND>(wId));
}

std::string screenTitle(webrtc::ScreenId sId)
{
    if (const auto hMonitor = hMonitorFromDeviceIndex(sId)) {
        MONITORINFOEXW info;
        info.cbSize = sizeof(MONITORINFOEXW);
        if (::GetMonitorInfoW(hMonitor, &info)) {
            return screenFriendlyTitle(info.szDevice);
        }
    }
    return std::string();
}

std::optional<std::string> windowTitle(webrtc::WindowId wId)
{
    if (HWND hwnd = reinterpret_cast<HWND>(wId)) {
        int length = ::GetWindowTextLengthW(hwnd);
        if (length++) {
            std::wstring result(length, '\0');
            length = ::GetWindowTextW(hwnd, result.data(), length);
            result.resize(length);
            return rtc::ToUtf8(result);
        }
    }
    return std::nullopt;
}

webrtc::DesktopSize screenResolution(const webrtc::DesktopCaptureOptions& options, webrtc::ScreenId sId)
{
    if (webrtc::kFullDesktopScreenId == sId) {
        return { ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
                ::GetSystemMetrics(SM_CYVIRTUALSCREEN) };
    }
    if (const auto hMonitor = hMonitorFromDeviceIndex(sId)) {
        MONITORINFOEXA info;
        info.cbSize = sizeof(MONITORINFOEXA);
        if (::GetMonitorInfoA(hMonitor, &info)) {
            return webrtc::DesktopSize{ std::abs(info.rcMonitor.left - info.rcMonitor.right),
                                        std::abs(info.rcMonitor.top - info.rcMonitor.bottom) };
        }
    }
    return {};
}

bool enumerateScreens(const webrtc::DesktopCaptureOptions& options, std::vector<std::string>& out)
{
    webrtc::DesktopCapturer::SourceList sources;
    if (webrtc::GetScreenList(&sources)) {
        out.clear();
        out.reserve(sources.size());
        for (const auto& source : sources) {
            out.push_back(screenIdToString(source.id));
        }
        return true;
    }
    return false;
}

bool enumerateWindows(const webrtc::DesktopCaptureOptions& options, std::vector<std::string>& out)
{
    webrtc::WindowCaptureHelperWin helper;
    webrtc::DesktopCapturer::SourceList sources;
    if (helper.EnumerateCapturableWindows(&sources, options.enumerate_current_process_windows())) {
        out.clear();
        out.reserve(sources.size());
        for (const auto& source : sources) {
            if (windowIsValidForPreview(reinterpret_cast<HWND>(source.id))) {
                out.push_back(windowIdToString(source.id));
            }
        }
        return true;
    }
    return false;
}

bool isWgcSupported(bool window)
{
#ifdef RTC_ENABLE_WIN_WGC
    static const auto osv = operatingSystemVersion();
    if (!webrtc::HasActiveDisplay()) {
        // There is a bug in `CreateForMonitor` that causes a crash if there are no
        // active displays. The crash was fixed in Win11, but we are still unable
        // to capture screens without an active display.
        if (!window) {
            return false;
        }
        // There is a bug in the DWM (Desktop Window Manager) that prevents it from
        // providing image data if there are no displays attached. This was fixed in
        // Windows 11.
        if (std::get<0>(osv) < 11) {
            return false;
        }
    }
    // A bug in the WGC API `CreateForMonitor` prevents capturing the entire
    // virtual screen (all monitors simultaneously), this was fixed in 20H1. Since
    // we can't assert that we won't be asked to capture the entire virtual
    // screen, we report unsupported so we can fallback to another capturer.
    // https://en.wikipedia.org/wiki/Windows_10,_version_2004#:~:text=Windows%2010%20May%202020%20Update,19041.
    if (!window && std::get<2>(osv) < 19041) {
        return false;
    }
    static const auto valid = testWgcFramework();
    return valid;
#endif
    return false;
}

} // namespace LiveKitCpp


#ifdef RTC_ENABLE_WIN_WGC
namespace webrtc {
    extern bool ResolveCoreWinRTDelayload();
}

namespace WGC = ABI::Windows::Graphics::Capture;
using Microsoft::WRL::ComPtr;
#endif

namespace
{

// https://code.woboq.org/qt5/qtbase/src/plugins/platforms/windows/qwindowscontext.cpp.html#511
const char* g_qtClassKeys[] = {
    "qwindowpopupdropshadow",
    "qwindowtooldropshadow",
    "qwindowtooltipdropshadow" };

bool windowIsValidForPreview(HWND hwnd)
{
    if (hwnd && !::GetParent(hwnd)) { // only root windows acceptable
        static thread_local char className[256] = "";
        const int classNameLength = ::GetClassNameA(hwnd, className, sizeof(className) / sizeof(className[0]));
        if (classNameLength > 0) {
            // check that windows is not QT popup (comboboxes dropdowns, tooltips)
            const auto classView = LiveKitCpp::toLower(std::string(className, classNameLength));
            for (const char* qtClassKey : g_qtClassKeys) {
                if (std::string::npos != classView.find(qtClassKey)) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

HMONITOR hMonitorFromDeviceIndex(webrtc::ScreenId sId)
{
    HMONITOR hMonitor = NULL;
    if (webrtc::GetHmonitorFromDeviceIndex(sId, &hMonitor)) {
        return hMonitor;
    }
    return NULL;
}

std::string screenFriendlyTitle(const wchar_t* name)
{
    if (name) {
        // https://social.msdn.microsoft.com/Forums/en-US/668e3cf9-4e00-4b40-a6f8-c7d2fc1afd39/how-can-i-retrieve-monitor-information?forum=windowsgeneraldevelopmentissues#be6e67f3-c2a7-4647-bb39-ed4fbec5dce4
        UINT32 requiredPaths, requiredModes;
        if (ERROR_SUCCESS == ::GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,
            &requiredPaths, &requiredModes)) {
            std::vector<DISPLAYCONFIG_PATH_INFO> paths(requiredPaths);
            std::vector<DISPLAYCONFIG_MODE_INFO> modes(requiredModes);
            if (ERROR_SUCCESS == ::QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &requiredPaths,
                paths.data(), &requiredModes, modes.data(), nullptr)) {
                for (const auto& path : paths) {
                    DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName;
                    sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                    sourceName.header.size = sizeof(sourceName);
                    sourceName.header.adapterId = path.sourceInfo.adapterId;
                    sourceName.header.id = path.sourceInfo.id;
                    if (ERROR_SUCCESS == ::DisplayConfigGetDeviceInfo(&sourceName.header) &&
                        0 == ::wcscmp(name, sourceName.viewGdiDeviceName)) {
                        DISPLAYCONFIG_TARGET_DEVICE_NAME name;
                        name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                        name.header.size = sizeof(name);
                        name.header.adapterId = path.sourceInfo.adapterId;
                        name.header.id = path.targetInfo.id;
                        if (ERROR_SUCCESS == ::DisplayConfigGetDeviceInfo(&name.header) &&
                            0 != ::wcslen(name.monitorFriendlyDeviceName)) {
                            return rtc::ToUtf8(name.monitorFriendlyDeviceName);
                        }
                        break;
                    }
                }
            }
        }
        std::string friendlyName = rtc::ToUtf8(name);
        if (!friendlyName.empty()) {
            if (0UL == friendlyName.rfind(g_devPrefix, 0UL)) {
                friendlyName.erase(0UL, sizeof(g_devPrefix) - 1UL);
            }
            return friendlyName;
        }
    }
    return {};
}

#ifdef RTC_ENABLE_WIN_WGC
constexpr wchar_t g_apiContract[] = L"Windows.Foundation.UniversalApiContract";
constexpr wchar_t g_wgcSessionType[] = L"Windows.Graphics.Capture.GraphicsCaptureSession";
constexpr UINT16 g_requiredApiContractVersion = 8U;

bool testWgcFramework()
{
    if (!webrtc::ResolveCoreWinRTDelayload()) {
        return false;
    }
    // We need to check if the WGC APIs are present on the system. Certain SKUs
    // of Windows ship without these APIs.
    ComPtr<ABI::Windows::Foundation::Metadata::IApiInformationStatics> apiInfoStatics;
    HRESULT hr = webrtc::GetActivationFactory<
        ABI::Windows::Foundation::Metadata::IApiInformationStatics,
        RuntimeClass_Windows_Foundation_Metadata_ApiInformation>(
            &apiInfoStatics);
    if (FAILED(hr)) {
        return false;
    }

    HSTRING apiContract;
    hr = webrtc::CreateHstring(g_apiContract, ::wcslen(g_apiContract), &apiContract);
    if (FAILED(hr)) {
        return false;
    }

    boolean isApiPresent;
    hr = apiInfoStatics->IsApiContractPresentByMajor(apiContract, g_requiredApiContractVersion, &isApiPresent);
    webrtc::DeleteHstring(apiContract);
    if (FAILED(hr) || !isApiPresent) {
        return false;
    }

    HSTRING wgcSessionType;
    hr = webrtc::CreateHstring(g_wgcSessionType, ::wcslen(g_wgcSessionType), &wgcSessionType);
    if (FAILED(hr)) {
        return false;
    }

    boolean isTypePresent;
    hr = apiInfoStatics->IsTypePresent(wgcSessionType, &isTypePresent);
    webrtc::DeleteHstring(wgcSessionType);
    if (FAILED(hr) || !isTypePresent) {
        return false;
    }

    // If the APIs are present, we need to check that they are supported.
    ComPtr<WGC::IGraphicsCaptureSessionStatics> captureSessionStatics;
    hr = webrtc::GetActivationFactory<WGC::IGraphicsCaptureSessionStatics,
        RuntimeClass_Windows_Graphics_Capture_GraphicsCaptureSession>(
            &captureSessionStatics);
    if (FAILED(hr)) {
        return false;
    }

    boolean isSupported;
    hr = captureSessionStatics->IsSupported(&isSupported);
    if (FAILED(hr) || !isSupported) {
        return false;
    }
    return true;
}
#endif

} // namespace