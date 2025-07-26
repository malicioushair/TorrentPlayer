cmake_minimum_required(VERSION 3.28)
project(TorrentPlayer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src)

execute_process(
    COMMAND conan install ${CMAKE_SOURCE_DIR} --build=missing --output-folder=${CMAKE_BINARY_DIR} -s build_type=Debug
)

include(src/TorrentDownloader/TorrentDownloader.cmake)

find_package(GLog REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Quick QuickLayouts REQUIRED)
qt_standard_project_setup()

qt_add_resources(QT_RESOURCES resources/qml.qrc)

set(SOURCES src/main.cpp
            src/GuiController/GuiController.cpp
            src/GuiController/GuiController.h)

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

qt_add_qml_module(torrentplayer_qml
    URI ${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        src/qml/main.qml
        src/qml/Player/Player.qml
    RESOURCES
)
target_link_libraries(TorrentPlayer PRIVATE torrentplayer_qml)

qt_finalize_executable(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --qmldir ${CMAKE_SOURCE_DIR} "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Running windeployqt..."
)
