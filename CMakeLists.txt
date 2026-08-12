cmake_minimum_required(VERSION 3.28)
project(TorrentPlayer VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src)

include(cmake/Helpers.cmake)

execute_process(
    COMMAND conan install ${CMAKE_SOURCE_DIR}
          --build=missing
          --output-folder=${CMAKE_BINARY_DIR}
          -s build_type=${CMAKE_BUILD_TYPE}
    COMMAND_ERROR_IS_FATAL ANY
)

include(src/TorrentDownloader/TorrentDownloader.cmake)
include(src/SubtitleLoader/SubtitleLoader.cmake)
include(src/App/App.cmake)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
if (APPLE AND FOR_APP_STORE)
    if (NOT MAC_APP_IDENTITY)
        message(FATAL_ERROR "MAC_APP_IDENTITY is required!")
    endif()
    qt_generate_deploy_qml_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
        DEPLOY_TOOL_OPTIONS
        "\"-codesign=${MAC_APP_IDENTITY}\""
    )
else()
    qt_generate_deploy_qml_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
    )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION  .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION .)
install(SCRIPT ${deploy_script})


if (ON_BUILD_SERVER)
    message("ON_BUILD_SERVER!")
    SignAndDeploy()
endif()

if(APPLE AND FOR_APP_STORE)
    configure_file(
        ${CMAKE_SOURCE_DIR}/resources/mac/TorrentPlayer.entitlements.in
        ${CMAKE_BINARY_DIR}/resources/mac/TorrentPlayer.entitlements
    )

    find_program(PRODUCTBUILD_EXECUTABLE productbuild REQUIRED)
    set(MAC_INSTALLER_IDENTITY "" CACHE STRING
        "Mac Installer Distribution certificate identity")
    set(APP_BUNDLE
        "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
    set(PKG_OUTPUT
        "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg")

    add_custom_target(appstore-pkg
        # remove the old bundle
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${APP_BUNDLE}
        # run cmake install to create the app bundle anew
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Release
        # copy apple provisioning profile without extended attributes
        # call `xattr -cr path/to/original_profile` if `cp -X` does not help
        COMMAND cp -X
            "${MAC_PROVISIONING_PROFILE}"
            "${APP_BUNDLE}/Contents/embedded.provisionprofile"
        # sign the bundle
        COMMAND /usr/bin/codesign
            --force
            --timestamp
            --entitlements
                "${CMAKE_BINARY_DIR}/resources/mac/TorrentPlayer.entitlements"
            --sign "${MAC_APP_IDENTITY}"
            ${APP_BUNDLE}
        # remove the old package
        COMMAND "${CMAKE_COMMAND}" -E rm -f "${PKG_OUTPUT}"
        # create a sign app .pkg
        COMMAND "${PRODUCTBUILD_EXECUTABLE}"
            --sign "${MAC_INSTALLER_IDENTITY}"
            --component "${APP_BUNDLE}" /Applications
            "${PKG_OUTPUT}"
        DEPENDS ${PROJECT_NAME}
        BYPRODUCTS "${PKG_OUTPUT}"
        USES_TERMINAL
        VERBATIM
        COMMENT "Creating Mac App Store package"
    )
endif()

if(WIN32)
    add_custom_target(wix-installer
        COMMAND powershell
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/build-wix-installer.ps1"
            -ProductVersion "${PROJECT_VERSION}"
            -BuildDir "${CMAKE_BINARY_DIR}"
            -BuildType "$<CONFIG>"
            -MsiOutput "${CMAKE_BINARY_DIR}/package/TorrentPlayer-Windows-x64.msi"
            -PackageDir "${CMAKE_BINARY_DIR}/package/TorrentPlayer"
            -SkipBuild
        DEPENDS TorrentPlayer
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        USES_TERMINAL
        VERBATIM
    )
endif()
