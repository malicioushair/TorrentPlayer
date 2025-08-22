cmake_minimum_required(VERSION 3.28)
project(TorrentPlayer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src)

execute_process(
    COMMAND conan install ${CMAKE_SOURCE_DIR} --build=missing --output-folder=${CMAKE_BINARY_DIR} -s build_type=Debug
)

include(src/TorrentDownloader/TorrentDownloader.cmake)
include(src/App/App.cmake)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
)

install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/install
        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install)

if (APPLE)
    install(TARGETS torrentplayer_qml
        LIBRARY DESTINATION TorrentPlayer.app/Contents/Frameworks
    )
else()
    install(TARGETS torrentplayer_qml)
endif()

install(SCRIPT ${deploy_script})
