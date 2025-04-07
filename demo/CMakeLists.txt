cmake_minimum_required(VERSION 3.16)

project(LiveKitClient VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPackageHandleStandardArgs)
find_package(LiveKitCppClient REQUIRED)
find_package(ZaphoydTppWebsockets REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)

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
        trackwrapper.cpp
        videotrackwrapper.cpp
        audiotrackwrapper.cpp
        cameratrackwrapper.cpp
        cameraoptions.cpp
        videooptions.cpp
)

qt_add_qml_module(${PROJECT_NAME} URI LiveKitClient VERSION 1.0
    QML_FILES
            Main.qml
            Client.qml
            ConnectForm.qml
            SessionForm.qml
            ChatView.qml
            AudioServiceControl.qml
    SOURCES
            demoapp.h
            mediadeviceinfo.h
            mediadevicesmodel.h
            sessionwrapper.h
            trackwrapper.h
            videotrackwrapper.h
            cameratrackwrapper.h
            audiotrackwrapper.h
            videooptions.h
            cameraoptions.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE)

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.LiveKit.${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_DISPLAY_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} - LiveKit client desktop demo app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE LiveKitCppClient ZaphoydTppWebsockets)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia LiveKitCppClient ZaphoydTppWebsockets
)

get_target_property(ZP_LOCATION ZaphoydTppWebsockets LOCATION)
get_target_property(LC_LOCATION LiveKitCppClient LOCATION)

if (APPLE)
    set(FRAMEWORKS_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks)
    file(COPY ${ZP_LOCATION} DESTINATION ${FRAMEWORKS_FOLDER})
    file(COPY ${LC_LOCATION} DESTINATION ${FRAMEWORKS_FOLDER})
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
endif()

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
