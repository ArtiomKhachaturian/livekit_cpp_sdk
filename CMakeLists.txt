cmake_minimum_required(VERSION 3.15)

# Project name
project("LiveKitCpp" LANGUAGES CXX VERSION 0.0.1)

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

set(SIGNALING_LIB ${PROJECT_NAME}Signaling)
set(RTC_LIB ${PROJECT_NAME}Rtc)

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
        set(PLATFORM_SRC_FOLDER nix)
    endif(WIN32)
endif(APPLE)

# Generate C++ code from proto files
# Find Protobuf (ensure it is installed)
# Specify search paths in CMAKE_PREFIX_PATH variable (if needed): "/path/to/module_a;/another/path/to/module_b"
find_package(Protobuf 5.29.3 REQUIRED)
find_package(absl REQUIRED)
find_package(utf8_range REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS}) # Include Protobuf directories

set(WEBRTC_INCLUDE_DIR CACHE PATH "Path to folder with WebRTC headers")
set(WEBRTC_LIB_DIR CACHE PATH "Path to folder with WebRTCC static libraries")

# common sources for both librarires
file(GLOB_RECURSE WEBSOCKETS_API_HEADERS ${CMAKE_SOURCE_DIR}/websockets_api/include/*.${HEADER_FILE_EXT})
file(GLOB_RECURSE BRICKS_API_HEADERS ${CMAKE_SOURCE_DIR}/bricks/include/*.${HEADER_FILE_EXT})

# Specify source files for signaling lib
file(GLOB_RECURSE SIGNALING_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/livekit/signaling/*.${HEADER_FILE_EXT})
file(GLOB SIGNALING_COMMON_HEADERS ${CMAKE_SOURCE_DIR}/src/signaling/*.${HEADER_FILE_EXT})
file(GLOB SIGNALING_COMMON_SOURCES ${CMAKE_SOURCE_DIR}/src/signaling/*.${SOURCE_FILE_EXT})
file(GLOB SIGNALING_PLATFORM_HEADERS ${CMAKE_SOURCE_DIR}/src/signaling/${PLATFORM_SRC_FOLDER}/*.${HEADER_FILE_EXT})
file(GLOB SIGNALING_PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/src/signaling/${PLATFORM_SRC_FOLDER}/*.${PLATFORM_SRC_EXT})

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

# Create a shared library
add_library(${SIGNALING_LIB} SHARED
    ${SIGNALING_PUBLIC_HEADERS}
    ${SIGNALING_COMMON_HEADERS}
    ${SIGNALING_COMMON_SOURCES}
    ${SIGNALING_PLATFORM_HEADERS}
    ${SIGNALING_PLATFORM_SOURCES}
    ${WEBSOCKETS_API_HEADERS}
    ${BRICKS_API_HEADERS}
    ${PROTO_SOURCES} # Add generated .cpp files
    ${PROTO_HEADERS})

source_group(TREE ${CMAKE_SOURCE_DIR}/include/livekit/signaling PREFIX "SignalingApi" FILES ${SIGNALING_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/signaling PREFIX "Signaling" FILES ${SIGNALING_COMMON_HEADERS} 
                                                                             ${SIGNALING_COMMON_SOURCES}
                                                                             ${SIGNALING_PLATFORM_HEADERS} 
                                                                             ${SIGNALING_PLATFORM_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/websockets_api/include PREFIX "WebsocketsAPI" FILES ${WEBSOCKETS_API_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/bricks/include PREFIX "Bricks" FILES ${BRICKS_API_HEADERS})

# Add include directories
target_include_directories(${SIGNALING_LIB} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/bricks/include
    ${CMAKE_SOURCE_DIR}/bricks/include/logger
    ${CMAKE_SOURCE_DIR}/bricks/include/threadsafety
    ${CMAKE_SOURCE_DIR}/src/signaling
    ${CMAKE_SOURCE_DIR}/websockets_api/include
    ${CMAKE_BINARY_DIR}
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
endif()
target_compile_definitions(${SIGNALING_LIB} PRIVATE -DLIVEKIT_SIGNALING_EXPORTS -DLIVEKIT_PROTOCOL_VERSION=${LIVEKIT_PROTOCOL_VERSION} 
                                                    -DPROJECT_VERSION="${PROJECT_VERSION}")
