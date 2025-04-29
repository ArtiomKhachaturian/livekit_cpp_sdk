cmake_minimum_required(VERSION 3.16)

project(LiveKitClientDemo VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPackageHandleStandardArgs)
find_package(LiveKitClient REQUIRED HINTS ${PACKAGES_PATHS_HINT})
find_package(ZaphoydTppWebsockets REQUIRED HINTS ${PACKAGES_PATHS_HINT})
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)

if(APPLE)
    set(CODE_SIGNING_CERT $ENV{CODE_SIGNING_CERT} CACHE PATH "The sign certificate name, like 'Apple Development: John Smith (N4SRYBN648)' for example")
endif(APPLE)

qt_standard_project_setup(REQUIRES 6.7.3)

qt_add_executable(${PROJECT_NAME})

qt_add_qml_module(${PROJECT_NAME} URI LiveKitClient VERSION 1.0
    QML_FILES
            Main.qml
            Client.qml
            ConnectForm.qml
            SessionForm.qml
            ChatView.qml
            AudioServiceControl.qml
            VideoRenderer.qml
            VideoDiagnosticsView.qml
            CameraModelComboBox.qml
            ParticipantView.qml
            CameraOptionsComboBox.qml
            TextPanel.qml
            TextMultiLine.qml
            ElementsGrid.qml
            SharingSelectionForm.qml
            MediaOptionsForm.qml
    SOURCES
            main.cpp
            demoapp.cpp
            logger.h
            logger.cpp
            safeobj.h
            mediadeviceinfo.cpp
            mediadevicesmodel.cpp
            session.cpp
            audiotrack.cpp
            localvideotrack.cpp
            cameraoptionsmodel.cpp
            videooptions.cpp
            itemmodel.h
            audiodevice.cpp
            localvideodevice.cpp
            videosource.cpp
            videomodels.cpp
            videotrack.cpp
            participant.cpp
            lockable.h
            localparticipant.cpp
            remoteparticipant.cpp
            demoapp.h
            mediadeviceinfo.h
            mediadevicesmodel.h
            session.h
            localvideotrack.h
            audiotrack.h
            videooptions.h
            cameraoptionsmodel.h
            audiodevice.h
            videosource.h
            videomodels.h
            localvideodevice.h
            videotrack.h
            participant.h
            localparticipant.h
            remoteparticipant.h
            audiorecordingoptions.h
            audiorecordingoptions.cpp
            fpsmeter.h
            fpsmeter.cpp
            videofilter.h
            videofilter.cpp
            grayscalevideofilter.h
            grayscalevideofilter.cpp
            sepiavideofilter.h
            sepiavideofilter.cpp
            pencilvideofilter.h
            pencilvideofilter.cpp
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

target_include_directories(${PROJECT_NAME} PRIVATE LiveKitClient::Signaling LiveKitClient::Rtc ZaphoydTppWebsockets)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia
                                             LiveKitClient::Signaling
                                             LiveKitClient::Rtc
                                             ZaphoydTppWebsockets)
if (APPLE) # TODO: add these copyings to post-build steps
function(copyDylibToBundle FILE APP_BUNDLE_PATH)
    if (EXISTS ${FILE})
        file(COPY ${ZP_LOCATION} DESTINATION ${APP_BUNDLE_PATH})
        get_filename_component(FILE ${FILE} NAME)
        message(STATUS "${FILE} has been copied to ${APP_BUNDLE_PATH}")
    else()
        message(WARNING "File ${FILE} doesn't exists")
    endif()
endfunction()

function(copImportedDylibToBundle TARGET APP_BUNDLE_PATH)
    get_target_property(FILE ${TARGET} LOCATION)
    copyDylibToBundle(${FILE} ${APP_BUNDLE_PATH})
endfunction()

function(copyFrameworksToBundle APP_BUNDLE_PATH) # TODO: change it to post-build steps
    set(APP_BUNDLE_PATH ${APP_BUNDLE_PATH}/Contents/Frameworks)
    if (NOT EXISTS ${APP_BUNDLE_PATH})
        file(MAKE_DIRECTORY ${APP_BUNDLE_PATH})
        message(STATUS "${APP_BUNDLE_PATH} has been created")
    endif()
    copImportedDylibToBundle(ZaphoydTppWebsockets ${APP_BUNDLE_PATH})
    copImportedDylibToBundle(LiveKitClient::Signaling ${APP_BUNDLE_PATH})
    copImportedDylibToBundle(LiveKitClient::Rtc ${APP_BUNDLE_PATH})
endfunction()

copyFrameworksToBundle(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app)
foreach(CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
    copyFrameworksToBundle(${CMAKE_CURRENT_BINARY_DIR}/${CONFIGURATION}/${PROJECT_NAME}.app)
endforeach()
set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
endif()

if(NOT "${CODE_SIGNING_CERT}" STREQUAL "")
   message(STATUS "Code signing added, for ${PROJECT_NAME}, with cert '${CODE_SIGNING_CERT}'")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
       COMMAND codesign --force --sign ${CODE_SIGNING_CERT} $<TARGET_FILE:${PROJECT_NAME}>
       COMMENT "Signing ${PROJECT_NAME}" VERBATIM
   )
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
