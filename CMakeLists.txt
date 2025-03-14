cmake_minimum_required(VERSION 3.15)

set(LIB_PROJECT_NAME LiveKitCppClient)

# Project name
project(${LIB_PROJECT_NAME} LANGUAGES CXX VERSION 0.0.1)

include(CMakePackageConfigHelpers)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(LIVEKIT_PROTOCOL_VERSION 12)

# Specify source files
file(GLOB_RECURSE PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE CPP_HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
if (APPLE)
    file(GLOB_RECURSE OBJC_SOURCES ${CMAKE_SOURCE_DIR}/src/*.mm)
    list(APPEND CPP_SOURCES ${OBJC_SOURCES})
endif(APPLE)

# Generate C++ code from proto files
# Find Protobuf (ensure it is installed)
# Add protobuf path into CMAKE_PREFIX_PATH if needed
find_package(Protobuf 5.29.3 REQUIRED)
find_package(absl REQUIRED)
find_package(utf8_range REQUIRED)
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

# Create a static library
add_library(${LIB_PROJECT_NAME} SHARED
    ${PUBLIC_HEADERS}
    ${CPP_HEADERS}
    ${CPP_SOURCES}
    ${PROTO_SOURCES} # Add generated .cpp files
    ${PROTO_HEADERS}
)

source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_SOURCES})
source_group(TREE ${CMAKE_BINARY_DIR} PREFIX "ProtoBufGenerated" FILES ${PROTO_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "PublicHeaders" FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Headers" FILES ${CPP_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Sources" FILES ${CPP_SOURCES})

# Add include directories
target_include_directories(${LIB_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/websockets_api/include
    ${CMAKE_SOURCE_DIR}/bricks/include
    ${CMAKE_SOURCE_DIR}/bricks/include/logger
    ${CMAKE_SOURCE_DIR}/bricks/include/threadsafety
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

# Link Protobuf library
target_link_libraries(${LIB_PROJECT_NAME} PUBLIC protobuf::libprotobuf-lite protobuf::libprotobuf protobuf::libprotoc)
# Link abseil
target_link_libraries(${LIB_PROJECT_NAME} PUBLIC absl::base absl::strings absl::status 
                      absl::statusor absl::flat_hash_map absl::log absl::log_internal_check_op)
target_precompile_headers(${LIB_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/AbslAliasing.h)
# Link UFT8_range
target_link_libraries(${LIB_PROJECT_NAME} PUBLIC utf8_range::utf8_validity utf8_range::utf8_range)
# Protocol versioning
target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DLIVEKIT_CLIENT_EXPORTS 
    -DLIVEKIT_PROTOCOL_VERSION=${LIVEKIT_PROTOCOL_VERSION})

# Generic attributes for the library
set_target_properties(${LIB_PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE YES SOVERSION ${PROJECT_VERSION} VERSION ${PROJECT_VERSION})
if (APPLE)
    set_target_properties(${LIB_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    target_link_libraries(${LIB_PROJECT_NAME} PRIVATE "-framework CoreServices -framework IOKit -framework CoreWLAN")
elseif(WIN32)
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()
target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DLIVEKIT_CLIENT_EXPORTS -DPROJECT_VERSION="${PROJECT_VERSION}")

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