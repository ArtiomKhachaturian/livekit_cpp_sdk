cmake_minimum_required(VERSION 3.15)

# Project name
project(LiveKitCppClient LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify source files
set(SOURCE_FILES)

# Generate C++ code from proto files
find_package(Protobuf REQUIRED) # Find Protobuf (ensure it is installed)
include_directories(${Protobuf_INCLUDE_DIRS}) # Include Protobuf directories

file(GLOB_RECURSE PROTO_FILES ${CMAKE_SOURCE_DIR}/*.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES}) # Generate .cpp and .h files from proto
message(STATUS "PROTO_SRCS = ${PROTO_SRCS}")

# Create a static library
add_library(LiveKitCppClient STATIC
    ${SOURCE_FILES}
    ${PROTO_SRCS} # Add generated .cpp files
)

# Add include directories
target_include_directories(LiveKitCppClient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Link Protobuf library
target_link_libraries(LiveKitCppClient PUBLIC ${Protobuf_LIBRARIES})