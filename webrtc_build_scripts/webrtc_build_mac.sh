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

# https://chromiumdash.appspot.com/releases?platform=Mac
# stable 137.0.7151.68
release_revision=cec4daea7ed5da94fc38d790bd12694c86865447

echo "Fetch WebRTC sources, revision $release_revision"
fetch --nohooks webrtc
gclient sync --with_branch_heads -r "${release_revision}"

echo "Build of WebRTC started"
cd $(pwd)/src

output_dir=$(pwd)/out/Default
echo "Output build directory: $output_dir"

is_debug=true
arch=$(uname -m)

if [[ "$arch" == "x86_64" ]]; then
    if [[ $(sysctl -n sysctl.proc_translated 2>/dev/null) == "1" ]]; then
        echo "Rosetta 2 (x86_64 on arm64), target CPU is x64"
    else
        echo "Target CPU is native x86_64"
    fi
    arch="x64"
elif [[ "$arch" == "arm64" ]]; then
    echo "Target CPU is native arm64"
else
    echo "Unknown architecture: $arch"
fi

echo "Generate config for WebRTC build"
gn gen ${output_dir} --args="is_debug=$is_debug enable_dsyms=true rtc_include_tests=false rtc_build_tools=false rtc_libvpx_build_vp9=true rtc_include_builtin_audio_codecs=true is_clang=true use_custom_libcxx=false is_component_build=false target_os=\"mac\" target_cpu=\"$arch\" ffmpeg_branding=\"Chrome\" use_rtti=true use_lld=false rtc_use_h264=true rtc_use_h265=true rtc_enable_protobuf=false"

echo "Build of WebRTC started"
ninja -C ${output_dir}

echo "Check of WebRTC build"
gn check ${output_dir}
