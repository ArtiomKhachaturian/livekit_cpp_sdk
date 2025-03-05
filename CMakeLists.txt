cmake_minimum_required(VERSION 3.15)

set(LIB_PROJECT_NAME LiveKitCppClient)
set(ZAPHOYD_TPP_PROJ ZaphoydTppWebSockets)

# Project name
project(${LIB_PROJECT_NAME} LANGUAGES CXX VERSION 0.0.1)

include(CMakePackageConfigHelpers)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


set(HEADER_FILE_EXT h)
set(SOURCE_FILE_EXT cpp)

if (APPLE)
    set(PLATFORM_SRC_EXT mm)
    set(PLATFORM_SRC_FOLDER mac)
else()
    set(PLATFORM_SRC_EXT ${SOURCE_FILE_EXT})
    if (WIN32)
        set(PLATFORM_SRC_FOLDER win)
    else()
        message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}")
    endif(WIN32)
endif(APPLE)

# Generate C++ code from proto files
find_package(Protobuf REQUIRED) # Find Protobuf (ensure it is installed)
find_package(absl REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS}) # Include Protobuf directories

# Specify source files
file(GLOB_RECURSE PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.${HEADER_FILE_EXT})
file(GLOB COMMON_HEADERS ${CMAKE_SOURCE_DIR}/src/*.${HEADER_FILE_EXT})
file(GLOB COMMON_SOURCES ${CMAKE_SOURCE_DIR}/src/*.${SOURCE_FILE_EXT})
file(GLOB_RECURSE SERVICE_HEADERS ${CMAKE_SOURCE_DIR}/src/service/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE SERVICE_SOURCES ${CMAKE_SOURCE_DIR}/src/service/*.${SOURCE_FILE_EXT})
file(GLOB_RECURSE PLATFORM_HEADERS ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER}/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})
file(GLOB_RECURSE WEBSOCKETS_API_HEADERS ${CMAKE_SOURCE_DIR}/websockets_api/include/*.${HEADER_FILE_EXT})

option(USE_ZAPHOYD_TPP_SOCKETS "Use of well-known Zaphoyd framework for websockets backend" ON)
set(WEBRTC_INCLUDE_DIR CACHE PATH "Path to folder with WebRTC headers")
set(WEBRTC_LIB_DIR CACHE PATH "Path to folder with WebRTCC static libraries")

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

function(findOpenSSL)
    find_package(OpenSSL)
    if (${OpenSSL_FOUND})
        message(STATUS "Found OpenSSL")
    else()
        set(OPENSSL_INCLUDE_DIRS $ENV{OPENSSL_INCLUDE_DIRS} CACHE PATH "Path to folder with OpenSSL headers")
        set(OPENSSL_LIB_DIR $ENV{OPENSSL_LIB_DIR} CACHE PATH "Path to folder with OpenSSL libraries")
        if (EXISTS ${OPENSSL_LIB_DIR})
            set(OPENSSL_LIBS ssl crypto)
            set(OPENSSL_LIBRARIES "")
            foreach(OPENSSL_LIB ${OPENSSL_LIBS})
                if (WIN32)
                    set(OPENSSL_LIB "${OPENSSL_LIB_DIR}\\lib${OPENSSL_LIB}.lib")
                else()
                    set(OPENSSL_LIB "${OPENSSL_LIB_DIR}/lib${OPENSSL_LIB}.dylib")
                endif()
                if (EXISTS ${OPENSSL_LIB})
                    list(APPEND OPENSSL_LIBRARIES ${OPENSSL_LIB})
                else()
                    message(FATAL_ERROR "OpenSSL library ${OPENSSL_LIB} was not found")
                endif()
            endforeach()
        else()
            message(FATAL_ERROR "Not found OpenSSL, try to set the path to OpenSSL lib folder in the system variable OPENSSL_LIB_DIR")
        endif()
    endif()
    set(OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES} PARENT_SCOPE)
    set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS} PARENT_SCOPE)
endfunction(findOpenSSL)

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/proto_files)
recreateDir(${GENERATED_DIR})

file(GLOB_RECURSE PROTO_FILES ${CMAKE_SOURCE_DIR}/protocol/*.proto)
file(GLOB_RECURSE PSRPC_PROTO_FILES ${CMAKE_SOURCE_DIR}/psrpc/protoc-gen-psrpc/*.proto)
list(APPEND PROTO_FILES ${PSRPC_PROTO_FILES})

# there is a huge problem with [protobuf_generate_cpp] function:
# it expects of flat structure for the folder with protofiles,
# see documentation at https://cmake.org/cmake/help/latest/module/FindProtobuf.html
# ("Note The protobuf_generate_cpp and protobuf_generate_python 
# functions and add_executable() or add_library() calls only work properly within the same directory.")
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(FILENAME ${PROTO_FILE} NAME)
    set(DEST_FILE ${GENERATED_DIR}/${FILENAME})
    configure_file(${PROTO_FILE} ${DEST_FILE} COPYONLY)
endforeach()
file(GLOB_RECURSE PROTO_FILES ${GENERATED_DIR}/*.proto)

# Generate .cpp and .h files from proto
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_FILES})

if (USE_ZAPHOYD_TPP_SOCKETS)
    findOpenSSL()
    option(ZAPHOYD_SHARED_IO_SERVICE "Common I/O worker thread for all websockets instances" ON)
    set(TPP_ZIP_URL "https://github.com/zaphoyd/websocketpp/archive/refs/heads/master.zip")
    set(TPP_SRC_DIR ${CMAKE_BINARY_DIR}/zaphoyd_tpp_src)
    set(ASIO_ZIP_URL "https://sourceforge.net/projects/asio/files/asio/1.30.2%20%28Stable%29/asio-1.30.2.tar.gz/download")
    set(ASIO_SRC_DIR ${CMAKE_BINARY_DIR}/asio_src)
    downloadAndExtract(${TPP_ZIP_URL} ${CMAKE_BINARY_DIR}/zaphoyd_tpp.zip ${TPP_SRC_DIR})
    downloadAndExtract(${ASIO_ZIP_URL} ${CMAKE_BINARY_DIR}/asio.zip ${ASIO_SRC_DIR})
    set(TPP_HDRS_DIR ${TPP_SRC_DIR}/websocketpp-master)
    set(ASIO_HDRS_DIR ${ASIO_SRC_DIR}/asio-1.30.2/include)
    file(GLOB_RECURSE ZAPHOYD_TPP_HEADERS ${CMAKE_SOURCE_DIR}/src/zaphoyd_tpp/*.${HEADER_FILE_EXT})
    file(GLOB_RECURSE ZAPHOYD_TPP_SOURCES ${CMAKE_SOURCE_DIR}/src/zaphoyd_tpp/*.${SOURCE_FILE_EXT})
endif(USE_ZAPHOYD_TPP_SOCKETS)

# WebRTC -> TODO: split WEBRTC_LIB_DIR for all configurations (debug/release/...)
if (EXISTS ${WEBRTC_INCLUDE_DIR} AND EXISTS ${WEBRTC_LIB_DIR})
    message(STATUS "Found WebRTC")
    set(WEBRTC_LIBS
        "audio_decoder_g711"
        "audio_decoder_g722"
        "audio_decoder_L16"
        "audio_decoder_opus"
        "builtin_audio_decoder_factory"
        "primitives"
        "desktop_capture_differ_sse2"
        "webrtc")
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
        message(STATUS "Found ${WEBRTC_LIB} static library")
        list(APPEND WEBRTC_LIB_FILES ${WEBRTC_LIB_FILE})
    else()
        message(FATAL_ERROR "Static library ${WEBRTC_LIB} was not found")
    endif()
    endforeach()
    set(WEBRTC_AVAILABLE TRUE)
    file(GLOB WEBRTC_HEADERS ${CMAKE_SOURCE_DIR}/src/webrtc/*.${HEADER_FILE_EXT})
    file(GLOB WEBRTC_SOURCES ${CMAKE_SOURCE_DIR}/src/webrtc/*.${SOURCE_FILE_EXT})
    file(GLOB WEBRTC_PLATFORM_HEADERS ${CMAKE_SOURCE_DIR}/src/webrtc/${PLATFORM_SRC_FOLDER}/*.${HEADER_FILE_EXT})
    file(GLOB WEBRTC_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/webrtc/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})
endif()

# Create a static library
add_library(${LIB_PROJECT_NAME} SHARED
    ${PUBLIC_HEADERS}
    ${COMMON_HEADERS}
    ${COMMON_SOURCES}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
    ${WEBSOCKETS_API_HEADERS}
    ${SERVICE_HEADERS}
    ${SERVICE_SOURCES}
    ${ZAPHOYD_TPP_HEADERS}
    ${ZAPHOYD_TPP_SOURCES}
    ${WEBRTC_HEADERS}
    ${WEBRTC_SOURCES}
    ${WEBRTC_PLATFORM_HEADERS}
    ${WEBRTC_PLATFORM_SOURCES}
    ${PROTO_SOURCES} # Add generated .cpp files
    ${PROTO_HEADERS}
)

source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "PublicHeaders" FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Sources" FILES ${COMMON_HEADERS} ${COMMON_SOURCES})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_HEADERS} ${PROTO_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER} PREFIX "Platform" FILES ${PLATFORM_HEADERS} ${PLATFORM_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/websockets_api/include PREFIX "WebsocketsAPI" FILES ${WEBSOCKETS_API_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/service PREFIX "Service" FILES ${SERVICE_HEADERS} ${SERVICE_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/zaphoyd_tpp PREFIX "ZaphoydTpp" FILES ${ZAPHOYD_TPP_HEADERS} ${ZAPHOYD_TPP_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/webrtc PREFIX "WebRTC" FILES ${WEBRTC_HEADERS} ${WEBRTC_SOURCES} 
    ${WEBRTC_PLATFORM_HEADERS} ${WEBRTC_PLATFORM_SOURCES})

# Add include directories
target_include_directories(${LIB_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_SOURCE_DIR}/websockets_api/include
    ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER}
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Websockets
if (USE_ZAPHOYD_TPP_SOCKETS)
    add_library(${ZAPHOYD_TPP_PROJ} SHARED
        ${ZAPHOYD_TPP_HEADERS}
        ${ZAPHOYD_TPP_SOURCES}
        ${WEBSOCKETS_API_HEADERS}
    )
    target_compile_definitions(${ZAPHOYD_TPP_PROJ} PRIVATE -DASIO_STANDALONE)
    if (WIN32)
        target_compile_definitions(${ZAPHOYD_TPP_PROJ} PRIVATE -D_WEBSOCKETPP_CPP11_STL_)
    endif()
    if (ZAPHOYD_SHARED_IO_SERVICE)
        target_compile_definitions(${ZAPHOYD_TPP_PROJ} PRIVATE -DWEBSOCKETS_TPP_SHARED_IO_SERVICE)
    endif(ZAPHOYD_SHARED_IO_SERVICE)
    target_include_directories(${ZAPHOYD_TPP_PROJ} PRIVATE ${TPP_HDRS_DIR} 
        ${ASIO_HDRS_DIR} ${OPENSSL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src/zaphoyd_tpp)
    if (WIN32)
        target_link_libraries(${ZAPHOYD_TPP_PROJ} PUBLIC "ws2_32.lib")
    endif(WIN32)
    target_link_libraries(${ZAPHOYD_TPP_PROJ} PUBLIC ${OPENSSL_LIBRARIES})
endif(USE_ZAPHOYD_TPP_SOCKETS)

# WebRTC
if (WEBRTC_AVAILABLE)
    target_include_directories(${LIB_PROJECT_NAME} PRIVATE ${WEBRTC_INCLUDE_DIR} 
        ${CMAKE_SOURCE_DIR}/src/webrtc ${CMAKE_SOURCE_DIR}/src/webrtc/${PLATFORM_SRC_FOLDER} 
        ${WEBRTC_INCLUDE_DIR}/third_party/abseil-cpp ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include)
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DWEBRTC_AVAILABLE)
    if (APPLE)
        target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DWEBRTC_POSIX -DWEBRTC_MAC)
        target_link_libraries(${LIB_PROJECT_NAME} PRIVATE "-framework IOSurface -framework Cocoa -framework CoreMedia -framework CoreVideo -framework CoreAudio -framework AVFoundation -framework AudioToolbox -framework AudioUnit -framework Security -framework SystemConfiguration")
    elseif(WIN32)
        target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DWEBRTC_WIN -D_CRT_SECURE_NO_WARNINGS -RTC_ENABLE_WIN_WGC)
    endif()
    target_link_libraries(${LIB_PROJECT_NAME} PRIVATE ${WEBRTC_LIB_FILES})
endif()

# Link Protobuf library
target_link_libraries(${LIB_PROJECT_NAME} PUBLIC ${Protobuf_LIBRARIES})
# Link abseil
target_link_libraries(${LIB_PROJECT_NAME} PUBLIC absl::base absl::strings absl::status absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)
# Generic attributes for the library
set_target_properties(${LIB_PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE YES SOVERSION ${PROJECT_VERSION} VERSION ${PROJECT_VERSION})
if (APPLE)
    set_target_properties(${LIB_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    target_link_libraries(${LIB_PROJECT_NAME} PRIVATE "-framework CoreServices -framework IOKit")
elseif(WIN32)
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DUNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()
target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DLIVEKIT_CLIENT_EXPORTS -DPROJECT_VERSION="${PROJECT_VERSION}")


# test application
if (USE_ZAPHOYD_TPP_SOCKETS)
    set(TEST_APP_NAME ${LIB_PROJECT_NAME}TestApp)
    file(GLOB_RECURSE CPP_SOURCES ${CMAKE_SOURCE_DIR}/test/*.${SOURCE_FILE_EXT})
    file(GLOB_RECURSE CPP_HEADERS ${CMAKE_SOURCE_DIR}/test/*.${HEADER_FILE_EXT})

    add_executable(${TEST_APP_NAME} 
        ${CPP_HEADERS}
        ${CPP_SOURCES}
    )

    target_include_directories(${TEST_APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${TEST_APP_NAME} PRIVATE ${LIB_PROJECT_NAME})
endif(USE_ZAPHOYD_TPP_SOCKETS)

# install steps
if (EXISTS ${CMAKE_INSTALL_PREFIX})
    install(TARGETS ${LIB_PROJECT_NAME} EXPORT ${LIB_PROJECT_NAME}-targets 
            ARCHIVE DESTINATION lib 
            LIBRARY DESTINATION lib 
            RUNTIME DESTINATION bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include DESTINATION .)
    # generate LiveKitCppClient-config.cmake and LiveKitCppClient-config-version.cmake files
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    configure_package_config_file("${LIB_PROJECT_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/${LIB_PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_PROJECT_NAME}")
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/generated/${LIB_PROJECT_NAME}-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    # install config and version file
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/${LIB_PROJECT_NAME}-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/generated/${LIB_PROJECT_NAME}-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_PROJECT_NAME}")
    # install targets file
    install(EXPORT "${LIB_PROJECT_NAME}-targets" 
        NAMESPACE "${LIB_PROJECT_NAME}::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_PROJECT_NAME}")
endif()