include(cmake/Helpers.cmake)

find_package(GLog REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Quick QuickLayouts REQUIRED)
qt_standard_project_setup()
qt_add_resources(QT_RESOURCES resources/qml.qrc)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${QT_RESOURCES})
if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
    )
endif()

qt6_import_qml_plugins(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Quick
    Qt6::QuickLayouts
    TorrentDownloader
    glog::glog
)

file(GLOB_RECURSE ABS_QML CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/qml/*.qml"
)

AbsToRelPath(REL_QML "${CMAKE_CURRENT_SOURCE_DIR}" ${ABS_QML})

qt_add_qml_module(torrentplayer_qml
    URI ${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        ${REL_QML}
    RESOURCES
)

target_link_libraries(${PROJECT_NAME} PRIVATE torrentplayer_qml)

qt_finalize_executable(${PROJECT_NAME})

target_link_libraries(TorrentDownloader
    LibtorrentRasterbar::torrent-rasterbar
    glog::glog
)