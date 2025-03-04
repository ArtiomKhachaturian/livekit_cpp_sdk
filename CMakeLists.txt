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

# Specify source files
file(GLOB_RECURSE PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE CPP_HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
if (APPLE)
    file(GLOB_RECURSE OBJC_SOURCES ${CMAKE_SOURCE_DIR}/src/*.mm)
    list(APPEND CPP_SOURCES ${OBJC_SOURCES})
endif(APPLE)

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
            #string(REPLACE ";" ";" OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES}")
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

option(USE_ZAPHOYD_TPP_SOCKETS "Use of well-known Zaphoyd framework for websockets backend" ON)

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
endif(USE_ZAPHOYD_TPP_SOCKETS)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/websocket
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS} # Add Protobuf include paths
)

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
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()
target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DLIVEKIT_CLIENT_EXPORTS -DPROJECT_VERSION="${PROJECT_VERSION}")

# Websockets
if (USE_ZAPHOYD_TPP_SOCKETS)
    target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DUSE_ZAPHOYD_TPP_SOCKETS -DASIO_STANDALONE)
    if (WIN32)
        target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -D_WEBSOCKETPP_CPP11_STL_)
    endif()
    if (ZAPHOYD_SHARED_IO_SERVICE)
        target_compile_definitions(${LIB_PROJECT_NAME} PRIVATE -DWEBSOCKETS_TPP_SHARED_IO_SERVICE)
    endif(ZAPHOYD_SHARED_IO_SERVICE)
    target_include_directories(${LIB_PROJECT_NAME} PRIVATE ${TPP_HDRS_DIR} ${ASIO_HDRS_DIR} ${OPENSSL_INCLUDE_DIRS})
    if (WIN32)
        target_link_libraries(${LIB_PROJECT_NAME} PUBLIC "ws2_32.lib")
    endif(WIN32)
    target_link_libraries(${LIB_PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})
endif(USE_ZAPHOYD_TPP_SOCKETS)

# test application
if (USE_ZAPHOYD_TPP_SOCKETS)
    set(TEST_APP_NAME ${LIB_PROJECT_NAME}TestApp)
    file(GLOB_RECURSE CPP_SOURCES ${CMAKE_SOURCE_DIR}/test/*.cpp)
    file(GLOB_RECURSE CPP_HEADERS ${CMAKE_SOURCE_DIR}/test/*.h)

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