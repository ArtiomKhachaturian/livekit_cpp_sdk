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

qt_add_qml_module(appLiveKitClient
    URI LiveKitClient
    VERSION 1.0
    QML_FILES
        Main.qml
        QML_FILES Client.qml
        QML_FILES ConnectForm.qml
        QML_FILES SessionForm.qml
        QML_FILES
        SOURCES demoapp.h demoapp.cpp
        SOURCES logger.h logger.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appLiveKitClient PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appLiveKitClient
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appLiveKitClient PRIVATE LiveKitCppClient ZaphoydTppWebsockets)

target_link_libraries(appLiveKitClient
    PRIVATE Qt6::Quick Qt6::QuickControls2 LiveKitCppClient ZaphoydTppWebsockets
)

include(GNUInstallDirs)
install(TARGETS appLiveKitClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
