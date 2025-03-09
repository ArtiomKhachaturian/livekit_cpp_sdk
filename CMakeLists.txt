cmake_minimum_required(VERSION 3.15)

# Project name
project("LiveKitCppClient" LANGUAGES CXX VERSION 0.0.1)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

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
# Specify search paths in CMAKE_PREFIX_PATH variable (if needed): "/path/to/module_a;/another/path/to/module_b"
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
file(GLOB_RECURSE BRICKS_API_HEADERS ${CMAKE_SOURCE_DIR}/bricks/include/*.${HEADER_FILE_EXT})

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

# WebRTC -> TODO: split WEBRTC_LIB_DIR for all configurations (debug/release/...)
if (WEBRTC_INCLUDE_DIR AND WEBRTC_LIB_DIR)
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

        set(NLOHMANN_ZIP_URL "https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip")
        set(NLOHMANN_SRC_DIR ${CMAKE_BINARY_DIR}/nlohmann_json_src)
        downloadAndExtract(${NLOHMANN_ZIP_URL} ${CMAKE_BINARY_DIR}/nlohmann_json.zip ${NLOHMANN_SRC_DIR})
        set(NLOHMANN_HDRS_DIR ${NLOHMANN_SRC_DIR}/single_include)

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

# Create a static library
add_library(${PROJECT_NAME} SHARED
    ${PUBLIC_HEADERS}
    ${COMMON_HEADERS}
    ${COMMON_SOURCES}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
    ${WEBSOCKETS_API_HEADERS}
    ${BRICKS_API_HEADERS}
    ${SERVICE_HEADERS}
    ${SERVICE_SOURCES}
    ${WEBRTC_HEADERS}
    ${WEBRTC_SOURCES}
    ${WEBRTC_PLATFORM_SOURCES}
    ${PROTO_SOURCES} # Add generated .cpp files
    ${PROTO_HEADERS}
)

source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "PublicHeaders" FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Sources" FILES ${COMMON_HEADERS} ${COMMON_SOURCES})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_HEADERS} ${PROTO_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER} PREFIX "Platform" FILES ${PLATFORM_HEADERS} ${PLATFORM_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/websockets_api/include PREFIX "WebsocketsAPI" FILES ${WEBSOCKETS_API_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/bricks/include PREFIX "Bricks" FILES ${BRICKS_API_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/service PREFIX "Service" FILES ${SERVICE_HEADERS} ${SERVICE_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/webrtc PREFIX "WebRTC" FILES ${WEBRTC_HEADERS} ${WEBRTC_SOURCES} 
    ${WEBRTC_PLATFORM_SOURCES})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_SOURCE_DIR}/websockets_api/include
    ${CMAKE_SOURCE_DIR}/bricks/include
    ${CMAKE_SOURCE_DIR}/bricks/include/logger
    ${CMAKE_SOURCE_DIR}/bricks/include/threadsafety
    ${CMAKE_SOURCE_DIR}/src/${PLATFORM_SRC_FOLDER}
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# WebRTC
if (WEBRTC_AVAILABLE)
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${WEBRTC_INCLUDE_DIR} 
        ${NLOHMANN_HDRS_DIR}
        ${CMAKE_SOURCE_DIR}/src/webrtc 
        ${CMAKE_SOURCE_DIR}/src/webrtc/${PLATFORM_SRC_FOLDER} 
        ${CMAKE_SOURCE_DIR}/src/webrtc/transport
        ${WEBRTC_INCLUDE_DIR}/third_party/abseil-cpp 
        ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DWEBRTC_AVAILABLE)
    if (APPLE)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DWEBRTC_POSIX -DWEBRTC_MAC)
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOSurface -framework Cocoa -framework CoreMedia -framework CoreVideo -framework CoreAudio -framework AVFoundation -framework AudioToolbox -framework AudioUnit -framework Security -framework SystemConfiguration")
    elseif(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DWEBRTC_WIN -D_CRT_SECURE_NO_WARNINGS -RTC_ENABLE_WIN_WGC)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WEBRTC_LIB_FILES})
endif()

# Link Protobuf library
target_link_libraries(${PROJECT_NAME} PRIVATE ${Protobuf_LIBRARIES})
# Link abseil
target_link_libraries(${PROJECT_NAME} PRIVATE absl::base absl::strings absl::status absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)
# Generic attributes for the library
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE YES SOVERSION ${PROJECT_VERSION} VERSION ${PROJECT_VERSION})
if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreServices -framework IOKit -framework CoreWLAN")
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE -DLIVEKIT_CLIENT_EXPORTS 
    -DLIVEKIT_PROTOCOL_VERSION=${LIVEKIT_PROTOCOL_VERSION} 
    -DPROJECT_VERSION="${PROJECT_VERSION}")

# install steps
if (CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    target_include_directories(${PROJECT_NAME} INTERFACE $<INSTALL_INTERFACE:include>)

    install(TARGETS ${PROJECT_NAME} 
            EXPORT ${PROJECT_NAME}Targets
            ARCHIVE DESTINATION lib 
            LIBRARY DESTINATION lib 
            RUNTIME DESTINATION bin)
    # public API installation
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include DESTINATION .)
    install(FILES ${WEBSOCKETS_API_HEADERS} DESTINATION ./include/websockets)
    install(FILES ${CMAKE_SOURCE_DIR}/bricks/include/Blob.h DESTINATION ./include)
    file(GLOB LOGGER_HEADERS ${CMAKE_SOURCE_DIR}/bricks/include/logger/*.${HEADER_FILE_EXT})
    install(FILES ${LOGGER_HEADERS} DESTINATION ./include/logger)

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
endif(CMAKE_INSTALL_PREFIX)