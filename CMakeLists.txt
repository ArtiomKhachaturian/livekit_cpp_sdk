cmake_minimum_required(VERSION 3.15)

# Project name
project("LiveKitClient" LANGUAGES CXX VERSION 0.0.1)
set(SIGNALING_LIB "Signaling")
set(RTC_LIB "Rtc")

# how to integrate to your project:
#       find_package(LiveKitClient REQUIRED)
#       target_include_directories(<your_target> PRIVATE LiveKitClient::Signaling LiveKitClient::Rtc)
#       target_link_libraries(<your_target> PRIVATE LiveKitClient::Signaling LiveKitClient::Rtc)
#       just for reminder - build of LiveKitClient::Rtc is optional

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(LIVEKIT_PROTOCOL_VERSION 12)

set(HEADER_FILE_EXT h)
set(SOURCE_FILE_EXT cpp)
set(PROTO_FILE_EXT proto)

if (APPLE)
    set(PLATFORM_SRC_EXT mm)
    set(PLATFORM_SRC_FOLDER mac)
else()
    set(PLATFORM_SRC_EXT ${SOURCE_FILE_EXT})
    if (WIN32)
        set(PLATFORM_SRC_FOLDER win)
    else()
        set(PLATFORM_SRC_FOLDER nix)
    endif(WIN32)
endif(APPLE)

# Signaling library for communicate with SFU throug PROTO messages

# Generate C++ code from proto files
# Find Protobuf (ensure it is installed)
# Specify search paths in CMAKE_PREFIX_PATH variable (if needed): "/path/to/module_a;/another/path/to/module_b"
find_package(absl REQUIRED)
find_package(utf8_range REQUIRED)
find_package(Protobuf 5.29.3 REQUIRED)

# common sources for both librarires
file(GLOB_RECURSE WEBSOCKETS_API_HEADERS ${CMAKE_SOURCE_DIR}/websockets_api/include/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE BRICKS_API_HEADERS ${CMAKE_SOURCE_DIR}/bricks/include/*.${HEADER_FILE_EXT})

if(APPLE)
    set(CODE_SIGNING_CERT $ENV{CODE_SIGNING_CERT} CACHE PATH "The sign certificate name, like 'Apple Development: John Smith (N4SRYBN648)' for example")
endif(APPLE)


function(recreateDir DIR)
    if (EXISTS ${DIR})
        file(REMOVE_RECURSE ${DIR})
    endif()
    file(MAKE_DIRECTORY ${DIR})
endfunction(recreateDir)

function(downloadAndExtract ZIP_URL ZIP_FILE EXTRACT_DIR)
    if (NOT EXISTS ${ZIP_FILE})
        message(STATUS "Downloading of ${ZIP_URL}")
        file(DOWNLOAD ${ZIP_URL} ${ZIP_FILE} SHOW_PROGRESS)
        recreateDir(${EXTRACT_DIR})
        file(ARCHIVE_EXTRACT INPUT ${ZIP_FILE} DESTINATION ${EXTRACT_DIR})
    endif()
endfunction(downloadAndExtract)

function(copyProtoFiles INPUT_DIR OUTPUT_DIR)
    file(GLOB_RECURSE PROTO_FILES ${INPUT_DIR}/*.${PROTO_FILE_EXT})
    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(FILENAME ${PROTO_FILE} NAME)
        set(DEST_FILE ${OUTPUT_DIR}/${FILENAME})
        configure_file(${PROTO_FILE} ${DEST_FILE} COPYONLY)
    endforeach()
endfunction()

function(signDylib target)
    if(NOT "${CODE_SIGNING_CERT}" STREQUAL "")
        message(STATUS "Code signing added, for ${target}, with cert '${CODE_SIGNING_CERT}'")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND codesign --force --sign ${CODE_SIGNING_CERT} $<TARGET_FILE:${target}>
            COMMENT "Signing ${target}" VERBATIM
        )
    endif()
endfunction()

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/proto_files)
recreateDir(${GENERATED_DIR})

# there is a huge problem with [protobuf_generate_cpp] function:
# it expects of flat structure for the folder with protofiles,
# see documentation at https://cmake.org/cmake/help/latest/module/FindProtobuf.html
# ("Note The protobuf_generate_cpp and protobuf_generate_python 
# functions and add_executable() or add_library() calls only work properly within the same directory.")
copyProtoFiles(${CMAKE_SOURCE_DIR}/protocol ${GENERATED_DIR})
copyProtoFiles(${CMAKE_SOURCE_DIR}/psrpc/protoc-gen-psrpc ${GENERATED_DIR})

file(GLOB_RECURSE PROTO_FILES ${GENERATED_DIR}/*.proto)

set(NLOHMANN_ZIP_URL "https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip")
set(NLOHMANN_SRC_DIR ${CMAKE_BINARY_DIR}/nlohmann_json_src)
downloadAndExtract(${NLOHMANN_ZIP_URL} ${CMAKE_BINARY_DIR}/nlohmann_json.zip ${NLOHMANN_SRC_DIR})
set(NLOHMANN_HDRS_DIR ${NLOHMANN_SRC_DIR}/single_include)

# Specify source files for signaling lib
file(GLOB_RECURSE SIGNALING_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/livekit/signaling/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE SIGNALING_COMMON_HEADERS ${CMAKE_SOURCE_DIR}/src/signaling/src/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE SIGNALING_COMMON_SOURCES ${CMAKE_SOURCE_DIR}/src/signaling/src/*.${SOURCE_FILE_EXT})
file(GLOB SIGNALING_PLATFORM_HEADERS ${CMAKE_SOURCE_DIR}/src/signaling/${PLATFORM_SRC_FOLDER}/*.${HEADER_FILE_EXT})
file(GLOB SIGNALING_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/signaling/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})

# Create a shared library
add_library(${SIGNALING_LIB} SHARED
    ${SIGNALING_PUBLIC_HEADERS}
    ${SIGNALING_COMMON_HEADERS}
    ${SIGNALING_COMMON_SOURCES}
    ${SIGNALING_PLATFORM_HEADERS}
    ${SIGNALING_PLATFORM_SOURCES}
    ${WEBSOCKETS_API_HEADERS}
    ${BRICKS_API_HEADERS}
    ${PROTO_FILES})

list(APPEND PROTO_IMPORT_DIRS ${GENERATED_DIR})
list(APPEND PROTO_IMPORT_DIRS ${Protobuf_INCLUDE_DIRS})
if (EXISTS ${Protobuf_DIR})
    set(Protobuf_INCLUDE_DIR "${Protobuf_DIR}/../../../include")
    file(REAL_PATH ${Protobuf_INCLUDE_DIR} Protobuf_INCLUDE_DIR)
    list(APPEND PROTO_IMPORT_DIRS ${Protobuf_INCLUDE_DIR})
endif()

protobuf_generate(TARGET ${SIGNALING_LIB} APPEND_PATH ${PROTO_IMPORT_DIRS} OUT_VAR PROTO_CPP IMPORT_DIRS ${PROTO_IMPORT_DIRS})

source_group(TREE ${CMAKE_SOURCE_DIR}/websockets_api/include PREFIX "WebsocketsAPI" FILES ${WEBSOCKETS_API_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/bricks/include PREFIX "Bricks" FILES ${BRICKS_API_HEADERS})

source_group(TREE ${CMAKE_SOURCE_DIR}/include/livekit/signaling PREFIX "SignalingApi" FILES ${SIGNALING_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/signaling/src PREFIX "Signaling" FILES ${SIGNALING_COMMON_HEADERS} 
                                                                                 ${SIGNALING_COMMON_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/signaling/${PLATFORM_SRC_FOLDER} PREFIX "Signaling" FILES ${SIGNALING_PLATFORM_HEADERS} ${SIGNALING_PLATFORM_SOURCES})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "Proto/Generated" FILES ${PROTO_CPP})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "Proto/Sources" FILES ${PROTO_FILES})

# Add include directories
target_include_directories(${SIGNALING_LIB} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/bricks/include
    ${CMAKE_SOURCE_DIR}/bricks/include/listeners
    ${CMAKE_SOURCE_DIR}/bricks/include/logger
    ${CMAKE_SOURCE_DIR}/bricks/include/threadsafety
    ${CMAKE_SOURCE_DIR}/src/signaling/src
    ${CMAKE_SOURCE_DIR}/websockets_api/include
    ${CMAKE_BINARY_DIR}
    ${NLOHMANN_HDRS_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Link Protobuf library
target_link_libraries(${SIGNALING_LIB} PRIVATE protobuf::libprotobuf-lite protobuf::libprotobuf protobuf::libprotoc)
# Link abseil
target_link_libraries(${SIGNALING_LIB} PRIVATE absl::base absl::strings absl::status 
                      absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)

# Link UFT8_range
target_link_libraries(${SIGNALING_LIB} PRIVATE utf8_range::utf8_validity utf8_range::utf8_range)
# Generic attributes for the library
set_target_properties(${SIGNALING_LIB} PROPERTIES POSITION_INDEPENDENT_CODE YES SOVERSION ${PROJECT_VERSION} VERSION ${PROJECT_VERSION})
if (WIN32)
    target_compile_definitions(${SIGNALING_LIB} PRIVATE -DUNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
    target_compile_options(${SIGNALING_LIB} PRIVATE /wd4068 /wd4141)
elseif (APPLE)
    set_target_properties(${SIGNALING_LIB} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    target_link_libraries(${SIGNALING_LIB} PRIVATE "-framework CoreServices -framework IOKit -framework CoreWLAN -framework SystemConfiguration")
    signDylib(${SIGNALING_LIB})
endif()
target_compile_definitions(${SIGNALING_LIB} PRIVATE -DLIVEKIT_SIGNALING_EXPORTS -DLIVEKIT_PROTOCOL_VERSION=${LIVEKIT_PROTOCOL_VERSION} 
                                                    -DPROJECT_VERSION="${PROJECT_VERSION}")

# RTC/WebRTC SDK
set(WEBRTC_INCLUDE_DIR CACHE PATH "Path to folder with WebRTC headers")
set(WEBRTC_LIB_DIR CACHE PATH "Path to folder with WebRTCC static libraries")

# WebRTC -> TODO: split WEBRTC_LIB_DIR for all configurations (debug/release/...)
if (WEBRTC_INCLUDE_DIR AND WEBRTC_LIB_DIR)
    if (EXISTS ${WEBRTC_INCLUDE_DIR} AND EXISTS ${WEBRTC_LIB_DIR})
        message(STATUS "Found WebRTC")
        set(WEBRTC_LIBS "webrtc" "common_video")
        if (APPLE)
            list(APPEND WEBRTC_LIBS
                "base_native_additions_objc"
                "base_objc"
                "callback_logger_objc"
                "default_codec_factory_objc"
                "file_logger_objc"
                "helpers_objc"
                "mediaconstraints_objc"
                "mediasource_objc"
                "metal_objc"
                "native_api"
                "native_video"
                "peerconnectionfactory_base_objc"
                "video_toolbox_cc"
                "videocapture_objc"
                "videocodec_objc"
                "videoframebuffer_objc"
                "videorendereradapter_objc"
                "videosource_objc"
                "videotoolbox_objc"
                "vp8"
                "vp9"
                "vpx_codec_constants"
                "wrapped_native_codec_objc")
        endif(APPLE)
        foreach(WEBRTC_LIB ${WEBRTC_LIBS})
        if (WIN32)
            set(WEBRTC_LIB ${WEBRTC_LIB}.lib)
        else()
            set(WEBRTC_LIB lib${WEBRTC_LIB}.a)
        endif()
        file(GLOB_RECURSE WEBRTC_LIB_FILE "${WEBRTC_LIB_DIR}/${WEBRTC_LIB}")
        if (WEBRTC_LIB_FILE)
            message(STATUS "Found ${WEBRTC_LIB} static library (${WEBRTC_LIB_FILE})")
            list(APPEND WEBRTC_LIB_FILES ${WEBRTC_LIB_FILE})
        else()
            message(FATAL_ERROR "Static library ${WEBRTC_LIB} was not found")
        endif()
        endforeach()

        set(WEBRTC_AVAILABLE TRUE)
        file(GLOB_RECURSE WEBRTC_HEADERS ${CMAKE_SOURCE_DIR}/src/webrtc/*.${HEADER_FILE_EXT})
        file(GLOB_RECURSE WEBRTC_SOURCES ${CMAKE_SOURCE_DIR}/src/webrtc/*.${SOURCE_FILE_EXT})
        file(GLOB WEBRTC_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/webrtc/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})
    else()
        if (NOT EXISTS ${WEBRTC_INCLUDE_DIR})
            message(FATAL_ERROR "WebRTC include folders ${WEBRTC_INCLUDE_DIR} doesn't exists")
        else()
            message(FATAL_ERROR "WebRTC libraries folders ${WEBRTC_LIB_DIR} doesn't exists")
        endif()
    endif()
endif()

if (WEBRTC_AVAILABLE)
    file(GLOB_RECURSE RTC_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/livekit/rtc/*.${HEADER_FILE_EXT})
    file(GLOB_RECURSE RTC_COMMON_HEADERS ${CMAKE_SOURCE_DIR}/src/rtc/src/*.${HEADER_FILE_EXT})
    file(GLOB_RECURSE RTC_COMMON_SOURCES ${CMAKE_SOURCE_DIR}/src/rtc/src/*.${SOURCE_FILE_EXT})
    file(GLOB_RECURSE RTC_PLATFORM_HEADERS ${CMAKE_SOURCE_DIR}/src/rtc/${PLATFORM_SRC_FOLDER}/*.${HEADER_FILE_EXT})
    file(GLOB_RECURSE RTC_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/rtc/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})

    set(OPENSSL_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/boringssl/src/include CACHE PATH "Path to folder with OpenSSL C++ headers")
    set(LIBYUV_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include CACHE PATH "Path to folder with LibYUV C++ headers")
    set(ABSEIL_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/abseil-cpp CACHE PATH "Path to folder with Abseil C++ headers")

    # Create a shared library
    add_library(${RTC_LIB} SHARED
        ${RTC_PUBLIC_HEADERS}
        ${RTC_COMMON_HEADERS}
        ${RTC_COMMON_SOURCES}
        ${RTC_PLATFORM_HEADERS}
        ${RTC_PLATFORM_SOURCES}
        ${WEBSOCKETS_API_HEADERS}
        ${BRICKS_API_HEADERS})

    if (APPLE)
        signDylib(${RTC_LIB})
    endif(APPLE)

    source_group(TREE ${CMAKE_SOURCE_DIR}/include/livekit/rtc PREFIX "RtcApi" FILES ${RTC_PUBLIC_HEADERS})
    source_group(TREE ${CMAKE_SOURCE_DIR}/src/rtc/src PREFIX "Rtc" FILES ${RTC_COMMON_HEADERS} 
                                                                         ${RTC_COMMON_SOURCES})
    source_group(TREE ${CMAKE_SOURCE_DIR}/src/rtc/${PLATFORM_SRC_FOLDER} PREFIX "Rtc/${PLATFORM_SRC_FOLDER}" FILES ${RTC_PLATFORM_HEADERS} ${RTC_PLATFORM_SOURCES})

    set(RTC_SRC_DIR ${CMAKE_SOURCE_DIR}/src/rtc)
    # Add include directories
    target_include_directories(${RTC_LIB} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/bricks/include
        ${CMAKE_SOURCE_DIR}/bricks/include/listeners
        ${CMAKE_SOURCE_DIR}/bricks/include/logger
        ${CMAKE_SOURCE_DIR}/bricks/include/threadsafety
        ${CMAKE_SOURCE_DIR}/websockets_api/include
        ${RTC_SRC_DIR}
        ${RTC_SRC_DIR}/src/utils
        ${RTC_SRC_DIR}/src/stats
        ${RTC_SRC_DIR}/src/media
        ${RTC_SRC_DIR}/src/media/audio
        ${RTC_SRC_DIR}/src/media/video
        ${RTC_SRC_DIR}/src/webrtc
        ${RTC_SRC_DIR}/src/webrtc/transport
        ${RTC_SRC_DIR}/src/webrtc/media
        ${RTC_SRC_DIR}/src/webrtc/media/timer
        ${RTC_SRC_DIR}/src/webrtc/media/e2e
        ${RTC_SRC_DIR}/src/webrtc/media/audio
        ${RTC_SRC_DIR}/src/webrtc/media/audio/adm
        ${RTC_SRC_DIR}/src/webrtc/media/data
        ${RTC_SRC_DIR}/src/webrtc/media/video
        ${RTC_SRC_DIR}/src/webrtc/media/video/buffers
        ${RTC_SRC_DIR}/src/webrtc/media/video/camera
        ${RTC_SRC_DIR}/src/webrtc/media/video/sharing
        ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/camera
        ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/sharing
        ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}
        ${ABSEIL_INCLUDE_DIR}
        ${LIBYUV_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${WEBRTC_INCLUDE_DIR}
    )

    set_target_properties(${RTC_LIB} PROPERTIES POSITION_INDEPENDENT_CODE YES SOVERSION ${PROJECT_VERSION} VERSION ${PROJECT_VERSION})

    if (APPLE)
        set_target_properties(${RTC_LIB} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
        target_include_directories(${RTC_LIB} PRIVATE 
            ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/video
            ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/video/camera
            ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/video/sharing
            ${RTC_SRC_DIR}/${PLATFORM_SRC_FOLDER}/video/sharing/sck
            ${WEBRTC_INCLUDE_DIR}/sdk/objc
            ${WEBRTC_INCLUDE_DIR}/sdk/objc/base
        )
        target_compile_definitions(${RTC_LIB} PRIVATE -DWEBRTC_POSIX -DWEBRTC_MAC)
        target_link_libraries(${RTC_LIB} PRIVATE "-framework CoreServices -framework IOKit -framework IOSurface -framework Cocoa -framework CoreMedia -framework CoreVideo -framework CoreAudio -framework AVFoundation -framework AudioToolbox -framework AudioUnit -framework Security -framework SystemConfiguration -framework ScreenCaptureKit")
    elseif(WIN32)
        target_compile_options(${RTC_LIB} PRIVATE /wd4068 /wd4141)
        target_compile_definitions(${RTC_LIB} PRIVATE -DWINRT_DIAGNOSTICS -DWEBRTC_WIN -D_CRT_SECURE_NO_WARNINGS -DRTC_ENABLE_WIN_WGC -DUNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
        # C++20 required for supporting of designated initializers (related to video codecs) in WebRTC code
        # maybe we need to review this approach and remove such initializer with replacement to own code
        target_compile_options(${RTC_LIB} PRIVATE /std:c++20)
        target_link_libraries(${RTC_LIB} PRIVATE wmcodecdspuuid.lib 
                                                 dmoguids.lib 
                                                 msdmo.lib 
                                                 avrt.lib 
                                                 strmiids.lib 
                                                 winmm.lib
                                                 windowscodecs.lib 
                                                 crypt32.lib 
                                                 secur32.lib 
                                                 ws2_32.lib 
                                                 d3d11.lib 
                                                 dxgi.lib 
                                                 iphlpapi.lib)
    endif()

    target_link_libraries(${RTC_LIB} PRIVATE ${WEBRTC_LIB_FILES})
    target_link_libraries(${RTC_LIB} PUBLIC ${SIGNALING_LIB})
    target_compile_definitions(${RTC_LIB} PRIVATE -DLIVEKIT_RTC_EXPORTS -DPROJECT_VERSION="${PROJECT_VERSION}")
endif()

# install steps
if (CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    set(CMAKE_INSTALL_INTERFACE_DIR ${CMAKE_INSTALL_PREFIX}/include)

    target_include_directories(${SIGNALING_LIB} INTERFACE ${CMAKE_INSTALL_INTERFACE_DIR})
    list(APPEND LIB_TARGETS ${SIGNALING_LIB})
    if (WEBRTC_AVAILABLE)
        list(APPEND LIB_TARGETS ${RTC_LIB})
        target_include_directories(${RTC_LIB} INTERFACE ${CMAKE_INSTALL_INTERFACE_DIR})
    endif()

    install(TARGETS ${LIB_TARGETS}
            EXPORT ${PROJECT_NAME}Targets
            ARCHIVE DESTINATION lib 
            LIBRARY DESTINATION lib 
            RUNTIME DESTINATION bin
            INCLUDES DESTINATION include)
    
    # public API installation
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include DESTINATION .)
    install(FILES ${WEBSOCKETS_API_HEADERS} DESTINATION ${CMAKE_INSTALL_INTERFACE_DIR}/websockets)
    install(FILES ${CMAKE_SOURCE_DIR}/bricks/include/Blob.h DESTINATION ${CMAKE_INSTALL_INTERFACE_DIR})
    file(GLOB LOGGER_HEADERS ${CMAKE_SOURCE_DIR}/bricks/include/logger/*.${HEADER_FILE_EXT})
    install(FILES ${LOGGER_HEADERS} DESTINATION ${CMAKE_INSTALL_INTERFACE_DIR}/logger)

    include(CMakePackageConfigHelpers)
    #include(GNUInstallDirs)

    # generate [config.cmake] and [config-version.cmake] files
    configure_package_config_file(./Config.cmake.in
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
      )
    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion)

    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    # install targets file
    install(EXPORT "${PROJECT_NAME}Targets" 
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    export(TARGETS ${LIB_TARGETS} NAMESPACE ${PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake)
endif(CMAKE_INSTALL_PREFIX)