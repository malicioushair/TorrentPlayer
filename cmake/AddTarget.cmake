include_guard(GLOBAL)

macro(AddTarget)
    set(_add_target_top_level_keywords
        TARGET_NAME
        TYPE
        AUTOMOC
        PACKAGE
        DEPENDENCIES
        COMPILE_DEFINITIONS
        ADDITIONAL_QML_SOURCES
    )
    set(_add_target_raw_arguments ${ARGN})
    set(_add_target_arguments)
    set(_add_target_package_count 0)
    list(LENGTH _add_target_raw_arguments _add_target_argument_count)
    set(_add_target_argument_index 0)

    while(_add_target_argument_index LESS _add_target_argument_count)
        list(GET _add_target_raw_arguments ${_add_target_argument_index} _add_target_token)
        if(_add_target_token STREQUAL "PACKAGE")
            math(EXPR _add_target_argument_index "${_add_target_argument_index} + 1")
            if(_add_target_argument_index GREATER_EQUAL _add_target_argument_count)
                message(FATAL_ERROR "AddTarget PACKAGE requires a package name")
            endif()

            list(GET _add_target_raw_arguments ${_add_target_argument_index} _add_target_package_name)
            if(_add_target_package_name IN_LIST _add_target_top_level_keywords
                OR _add_target_package_name STREQUAL "COMPONENTS")
                message(FATAL_ERROR "AddTarget PACKAGE requires a package name")
            endif()

            set(_add_target_package_${_add_target_package_count}_name "${_add_target_package_name}")
            set(_add_target_package_${_add_target_package_count}_components)
            math(EXPR _add_target_argument_index "${_add_target_argument_index} + 1")

            if(_add_target_argument_index LESS _add_target_argument_count)
                list(GET _add_target_raw_arguments ${_add_target_argument_index} _add_target_token)
                if(_add_target_token STREQUAL "COMPONENTS")
                    math(EXPR _add_target_argument_index "${_add_target_argument_index} + 1")
                    while(_add_target_argument_index LESS _add_target_argument_count)
                        list(GET _add_target_raw_arguments ${_add_target_argument_index} _add_target_token)
                        if(_add_target_token IN_LIST _add_target_top_level_keywords)
                            break()
                        endif()

                        list(APPEND
                            _add_target_package_${_add_target_package_count}_components
                            "${_add_target_token}"
                        )
                        math(EXPR _add_target_argument_index "${_add_target_argument_index} + 1")
                    endwhile()

                    if(NOT _add_target_package_${_add_target_package_count}_components)
                        message(FATAL_ERROR
                            "AddTarget PACKAGE ${_add_target_package_name} COMPONENTS requires at least one component"
                        )
                    endif()
                endif()
            endif()

            math(EXPR _add_target_package_count "${_add_target_package_count} + 1")
        else()
            list(APPEND _add_target_arguments "${_add_target_token}")
            math(EXPR _add_target_argument_index "${_add_target_argument_index} + 1")
        endif()
    endwhile()

    cmake_parse_arguments(
        ARG
        "AUTOMOC"
        "TARGET_NAME;TYPE"
        "DEPENDENCIES;COMPILE_DEFINITIONS;ADDITIONAL_QML_SOURCES"
        ${_add_target_arguments}
    )

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "AddTarget received unknown arguments: ${ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "AddTarget arguments are missing values: ${ARG_KEYWORDS_MISSING_VALUES}"
        )
    endif()

    if(NOT ARG_TARGET_NAME)
        message(FATAL_ERROR "AddTarget requires TARGET_NAME")
    endif()

    if(NOT ARG_TYPE)
        message(FATAL_ERROR "AddTarget requires TYPE")
    endif()

    if(_add_target_package_count GREATER 0)
        math(EXPR _add_target_last_package_index "${_add_target_package_count} - 1")
        foreach(_add_target_package_index RANGE 0 ${_add_target_last_package_index})
            set(_add_target_package_name_variable
                "_add_target_package_${_add_target_package_index}_name"
            )
            set(_add_target_package_components_variable
                "_add_target_package_${_add_target_package_index}_components"
            )
            set(_add_target_package_name "${${_add_target_package_name_variable}}")
            set(_add_target_package_components "${${_add_target_package_components_variable}}")

            if(_add_target_package_components)
                find_package(
                    ${_add_target_package_name}
                    CONFIG REQUIRED
                    COMPONENTS ${_add_target_package_components}
                )
            else()
                find_package(${_add_target_package_name} CONFIG REQUIRED)
            endif()

            unset(${_add_target_package_name_variable})
            unset(${_add_target_package_components_variable})
        endforeach()
    endif()

    file(GLOB_RECURSE _add_target_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/*.h"
    )

    if(ARG_TYPE STREQUAL "STATIC_LIB")
        add_library(
            ${ARG_TARGET_NAME}
            STATIC
            ${_add_target_sources}
        )
    elseif(ARG_TYPE STREQUAL "QT_EXECUTABLE")
        qt_standard_project_setup()

        if(QT_KNOWN_POLICY_QTP0004)
            qt_policy(SET QTP0004 NEW)
        endif()

        qt_add_executable(
            ${ARG_TARGET_NAME}
            ${_add_target_sources}
        )

        # Used only by the non-NDEBUG branch in GuiController.
        target_compile_definitions(
            ${ARG_TARGET_NAME}
            PRIVATE
                MAIN_QML="${CMAKE_CURRENT_LIST_DIR}/qml/Main.qml"
        )

        file(GLOB_RECURSE _add_target_abs_qml CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_LIST_DIR}/qml/*.qml"
            "${CMAKE_CURRENT_LIST_DIR}/qml/*.js"
        )

        AbsToRelPath(
            _add_target_rel_qml
            "${CMAKE_CURRENT_SOURCE_DIR}"
            ${_add_target_abs_qml}
        )

        set(QT_QML_GENERATE_QMLLS_INI ON)

        qt_add_qml_module(
            ${ARG_TARGET_NAME}
            URI ${PROJECT_NAME}
            VERSION 1.0
            RESOURCE_PREFIX "/qt/qml"
            QML_FILES
                ${_add_target_rel_qml}
            SOURCES
                ${ARG_ADDITIONAL_QML_SOURCES}
        )

    elseif(ARG_TYPE STREQUAL "UNIT_TESTS")
        include(GoogleTest)
        add_executable(${ARG_TARGET_NAME} ${CMAKE_SOURCE_DIR}/tests/${ARG_TARGET_NAME}.cpp)
        gtest_discover_tests(SubtitleLoaderTests)
    else()
        message(FATAL_ERROR
            "AddTarget received unsupported TYPE: ${ARG_TYPE}"
        )
    endif()

    if(ARG_AUTOMOC OR ARG_TYPE STREQUAL "QT_EXECUTABLE")
        set_target_properties(
            ${ARG_TARGET_NAME}
            PROPERTIES
                AUTOMOC ON
        )
    endif()

    if(ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(
            ${ARG_TARGET_NAME}
            PRIVATE
                ${ARG_COMPILE_DEFINITIONS}
        )
    endif()

    if(ARG_DEPENDENCIES)
        target_link_libraries(
            ${ARG_TARGET_NAME}
            PRIVATE
                ${ARG_DEPENDENCIES}
        )
    endif()

    unset(_add_target_abs_qml)
    unset(_add_target_argument_count)
    unset(_add_target_argument_index)
    unset(_add_target_arguments)
    unset(_add_target_last_package_index)
    unset(_add_target_package_components)
    unset(_add_target_package_components_variable)
    unset(_add_target_package_count)
    unset(_add_target_package_index)
    unset(_add_target_package_name)
    unset(_add_target_package_name_variable)
    unset(_add_target_raw_arguments)
    unset(_add_target_rel_qml)
    unset(_add_target_sources)
    unset(_add_target_token)
    unset(_add_target_top_level_keywords)
    unset(ARG_ADDITIONAL_QML_SOURCES)
    unset(ARG_AUTOMOC)
    unset(ARG_COMPILE_DEFINITIONS)
    unset(ARG_DEPENDENCIES)
    unset(ARG_KEYWORDS_MISSING_VALUES)
    unset(ARG_TARGET_NAME)
    unset(ARG_TYPE)
    unset(ARG_UNPARSED_ARGUMENTS)
endmacro()
