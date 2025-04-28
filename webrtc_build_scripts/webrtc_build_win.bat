@echo off

set script_path=%cd%

if exist webrtc-checkout (
    echo WebRTC checkout folder already present
) else (
    mkdir webrtc-checkout
)
cd webrtc-checkout

REM Fetch depot tools
if exist depot_tools (
    echo Depot tools folder is already present
) else (
    mkdir depot_tools
    cd depot_tools
    echo Pull of depot tools...
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git .
    cd ..
)

cd depot_tools
set depot_tools_path=%cd%
cd ..

echo Set path of depot tools to sys PATH variable - %depot_tools_path%
set PATH=%depot_tools_path%;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set GYP_MSVS_VERSION=2019

REM Revision of WebRTC sources
set release_revision=9e5db68b15087eccd8d2493b4e8539c1657e0f75

echo Fetch WebRTC sources, revision %release_revision%
call fetch --nohooks webrtc
call gclient sync --with_branch_heads -r %release_revision%

echo Go to the 'build' subfolder
cd src

cd ./build
echo Applying desktop_windows_crt.patch...
call git apply -v %script_path%\desktop_windows_crt.patch >nul 2>&1
if errorlevel 1 (
  echo Failed to apply desktop_windows_crt.patch . Abort execution.
  exit /b 1
)
cd ..

set output_dir=%cd%\out\Default
echo Output build directory: %output_dir%

set is_debug=true

echo Generate config for WebRTC build
call gn gen %output_dir% --args="use_rtti=true libcxx_is_shared=true use_custom_libcxx=false is_debug=%is_debug% rtc_include_tests=false rtc_build_tools=false rtc_libvpx_build_vp9=true rtc_include_builtin_audio_codecs=true is_component_build=false target_cpu=\"x64\"rtc_use_h264=true rtc_use_h265=true rtc_enable_protobuf=false"

echo Build of WebRTC started
call ninja -C %output_dir%

echo Check of WebRTC build
call gn check %output_dir%