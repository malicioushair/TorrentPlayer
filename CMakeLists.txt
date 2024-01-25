cmake_minimum_required(VERSION 3.28)
project(hello_qml VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

execute_process(
    COMMAND conan install ${CMAKE_SOURCE_DIR} --build=missing --output-folder=${CMAKE_BINARY_DIR}/ConanDeps -s build_type=Debug
)

find_package(Qt6 COMPONENTS Core Gui Quick  REQUIRED)
qt_standard_project_setup()

qt_add_resources(QT_RESOURCES resources/qml.qrc)

add_executable(${PROJECT_NAME} src/main.cpp ${QT_RESOURCES})
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --qmldir ${CMAKE_SOURCE_DIR} "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Running windeployqt..."
)
