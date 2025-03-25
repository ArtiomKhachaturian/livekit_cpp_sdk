#!/bin/sh

if [ -d ./webrtc-checkout ]; then
    echo "WebRTC checkout folder already present"
else
    mkdir ./webrtc-checkout
fi
cd ./webrtc-checkout

# fetch depot tools
if [ -d ./depot_tools ]; then
    echo "Depot tools folder is already present"
else
	mkdir ./depot_tools
	cd ./depot_tools
    echo "Pull of depot tools ..."
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git .
    cd ..
fi
cd ./depot_tools
depot_tools_path=$(pwd)
cd ..
echo "Set path of depot tools to sys PATH variable - $depot_tools_path"
export PATH=$depot_tools_path:$PATH

echo "Fetch WebRTC sources"
fetch --nohooks webrtc
gclient sync

gn_args='is_debug=true enable_dsyms=true rtc_include_tests=false rtc_include_builtin_audio_codecs=true rtc_build_tools=false is_clang=true use_custom_libcxx=false is_component_build=false target_os="mac" target_cpu="x64" use_rtti=true use_lld=false rtc_use_h265=true'

echo "Build of WebRTC started"
cd ./src
gn gen out/Default --args=$gn_args
ninja -C out/Default
