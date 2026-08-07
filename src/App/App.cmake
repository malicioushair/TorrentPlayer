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

    ADDITIONAL_QML_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/App/Controllers/SubtitlesController/SubtitlesController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/App/Controllers/SubtitlesController/SubtitlesController.cpp
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
