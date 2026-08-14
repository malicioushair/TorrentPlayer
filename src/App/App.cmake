include(cmake/Helpers.cmake)
include(cmake/AddTarget.cmake)

AddTarget(
    TARGET_NAME ${PROJECT_NAME}
    TYPE QT_EXECUTABLE

    PACKAGE Qt6
        COMPONENTS
            Core
            Gui
            Quick
            QuickEffects
            QuickLayouts
            QuickControls2
            Multimedia
            Network
            LinguistTools
    PACKAGE glog
    PACKAGE keychain

    DEPENDENCIES
        Qt6::Multimedia
        Qt6::Quick
        Qt6::QuickEffects
        Qt6::QuickLayouts
        Qt6::QuickControls2
        TorrentDownloader
        glog::glog
        keychain::keychain
        TorrentPlayerSubtitles

    COMPILE_DEFINITIONS
        $<IF:$<PLATFORM_ID:Darwin>,DISABLE_QT_HW_TEXTURE_CONVERSION,> # Workaround for a memory leak due to how Qt handles Metal textures on MacOS

    ADDITIONAL_QML_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/App/Controllers/SubtitlesController/SubtitlesController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/App/Controllers/SubtitlesController/SubtitlesController.cpp
)

file(GLOB TP_TS_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/translations/*.ts)
qt_add_translations(${PROJECT_NAME}
    SOURCE_TARGETS ${PROJECT_NAME} TorrentPlayerSubtitles
    TS_FILES ${TP_TS_FILES}
    LUPDATE_OPTIONS -no-obsolete
)

target_include_directories(${PROJECT_NAME} PRIVATE
# @TODO: make it more qt-way. Creating a path App/TorrentPlayer should help
    ${CMAKE_CURRENT_LIST_DIR}/Controllers/SubtitlesController
)

if (APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/resources/mac/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
    set(APP_ICON resources/mac/TorrentPlayer.icns)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_ICON_FILE "TorrentPlayer"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist
    )
    set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
elseif(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/resources/windows/TorrentPlayer.rc
    )
endif()

AddTarget(
    TARGET_NAME SubtitleLoaderTests
    TYPE UNIT_TESTS

    PACKAGE GTest

    DEPENDENCIES
        GTest::gtest_main
        Qt6::Core
        TorrentPlayerSubtitles
)
