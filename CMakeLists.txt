cmake_minimum_required(VERSION 3.15)

set(LIB_PROJECT_NAME LiveKitCppClient)

# Project name
project(${LIB_PROJECT_NAME} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify source files
file(GLOB_RECURSE INCLUDE_FILES ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Generate C++ code from proto files
find_package(Protobuf REQUIRED) # Find Protobuf (ensure it is installed)
find_package(absl REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS}) # Include Protobuf directories

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

option(USE_ZAPHOYD_TPP_SOCKETS "Use of well-known Zaphoyd framework for websockets backend" OFF)
#set(TPP_HDRS_DIR)
#set(ASIO_HDRS_DIR)

if(USE_ZAPHOYD_TPP_SOCKETS)
    set(TPP_ZIP_URL "https://github.com/zaphoyd/websocketpp/archive/refs/heads/master.zip")
    set(TPP_SRC_DIR ${CMAKE_BINARY_DIR}/zaphoyd_tpp_src)
    set(ASIO_ZIP_URL "https://sourceforge.net/projects/asio/files/asio/1.30.2%20%28Stable%29/asio-1.30.2.tar.gz/download")
    set(ASIO_SRC_DIR ${CMAKE_BINARY_DIR}/asio_src)
    downloadAndExtract(${TPP_ZIP_URL} ${CMAKE_BINARY_DIR}/zaphoyd_tpp.zip ${TPP_SRC_DIR})
    downloadAndExtract(${ASIO_ZIP_URL} ${CMAKE_BINARY_DIR}/asio.zip ${ASIO_SRC_DIR})
    set(TPP_HDRS_DIR ${TPP_SRC_DIR}/websocketpp-master)
    set(ASIO_HDRS_DIR ${ASIO_SRC_DIR}/asio-1.30.2/include)
endif(USE_ZAPHOYD_TPP_SOCKETS)

# Create a static library
add_library(${LIB_PROJECT_NAME} SHARED
    ${INCLUDE_FILES}
    ${SOURCE_FILES}
    ${PROTO_SOURCES} # Add generated .cpp files
    ${PROTO_HEADERS}
)

source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_SOURCES})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "Headers" FILES ${INCLUDE_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Sources" FILES ${SOURCE_FILES})

# Add include directories
target_include_directories(${LIB_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/websocket
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Link Protobuf library
target_link_libraries(${LIB_PROJECT_NAME} PRIVATE ${Protobuf_LIBRARIES})
# Link abseil
target_link_libraries(${LIB_PROJECT_NAME} PRIVATE absl::base absl::strings absl::status absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)
# Websockets
if(USE_ZAPHOYD_TPP_SOCKETS)
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DUSE_ZAPHOYD_TPP_SOCKETS)
    target_include_directories(${LIB_PROJECT_NAME} PRIVATE ${TPP_HDRS_DIR} ${ASIO_HDRS_DIR})
    if(WIN32)
        target_link_libraries(${LIB_PROJECT_NAME} PRIVATE "ws2_32")
    endif(WIN32)
endif(USE_ZAPHOYD_TPP_SOCKETS)