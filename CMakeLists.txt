cmake_minimum_required(VERSION 3.15)

# Project name
project(LiveKitCppClient LANGUAGES CXX)

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

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/proto_files)
if (EXISTS ${GENERATED_DIR})
    file(REMOVE_RECURSE ${GENERATED_DIR})
endif()
file(MAKE_DIRECTORY ${GENERATED_DIR})

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

# Create a static library
add_library(LiveKitCppClient SHARED
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
target_include_directories(LiveKitCppClient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Link Protobuf library
target_link_libraries(LiveKitCppClient PUBLIC ${Protobuf_LIBRARIES})
target_link_libraries(LiveKitCppClient PUBLIC absl::base absl::strings absl::status absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)