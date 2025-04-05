cmake_minimum_required(VERSION 3.16)

project(LiveKitClient VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPackageHandleStandardArgs)
find_package(LiveKitCppClient REQUIRED)
find_package(ZaphoydTppWebsockets REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appLiveKitClient
    main.cpp
)

qt_add_qml_module(appLiveKitClient URI LiveKitClient VERSION 1.0
    QML_FILES Main.qml
              Client.qml
              ConnectForm.qml
              SessionForm.qml
              ChatView.qml
    SOURCES demoapp.h demoapp.cpp
            logger.h logger.cpp
            safeobj.h
            mediadeviceinfo.h mediadeviceinfo.cpp
            mediadevicesmodel.h mediadevicesmodel.cpp
            sessionwrapper.h sessionwrapper.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appLiveKitClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(APPLE)
    #set(MAC_OS_LINK_RPATH "@executable_path")
    set_target_properties(appLiveKitClient PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
  endif(APPLE)

target_include_directories(appLiveKitClient PRIVATE LiveKitCppClient ZaphoydTppWebsockets)

target_link_libraries(appLiveKitClient
    PUBLIC Qt6::Quick Qt6::QuickControls2 LiveKitCppClient ZaphoydTppWebsockets
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe"
                --verbose 1
                --no-compiler-runtime
                --no-system-d3d-compiler
                --qmldir "${CMAKE_SOURCE_DIR}"
                \"$<TARGET_FILE:${PROJECT_NAME}>\"
        COMMENT "Deploying Qt libraries using windeployqt for compilation target '${PROJECT_NAME}' ..."
    )
elseif (MACOS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/macdeployqt"
              --verbose 1
              --no-compiler-runtime
              --qmldir "${CMAKE_SOURCE_DIR}"
              \"$<TARGET_FILE:${PROJECT_NAME}>\"
        COMMENT "Deploying Qt libraries using macdeployqt for compilation target '${PROJECT_NAME}' ..."
    )
endif()

include(GNUInstallDirs)
install(TARGETS appLiveKitClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
