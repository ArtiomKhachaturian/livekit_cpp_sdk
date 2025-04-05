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

qt_add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        demoapp.cpp
        logger.h
        logger.cpp
        safeobj.h
        mediadeviceinfo.cpp
        mediadevicesmodel.cpp
        sessionwrapper.cpp
)

qt_add_qml_module(${PROJECT_NAME} URI LiveKitClient VERSION 1.0
    QML_FILES
            Main.qml
            Client.qml
            ConnectForm.qml
            SessionForm.qml
            ChatView.qml
    SOURCES
            demoapp.h
            mediadeviceinfo.h
            mediadevicesmodel.h
            sessionwrapper.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    #MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(${PROJECT_NAME} PRIVATE LiveKitCppClient ZaphoydTppWebsockets)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Qt6::Quick Qt6::QuickControls2 LiveKitCppClient ZaphoydTppWebsockets
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
