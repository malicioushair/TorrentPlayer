include(cmake/AddTarget.cmake)

AddTarget(
    TARGET_NAME TorrentPlayerSubtitles
    TYPE SHARED_LIB
    AUTOMOC

    PACKAGE Qt6
        COMPONENTS
            Core
            Network
    PACKAGE ZLIB

    DEPENDENCIES
        Qt6::Core
        Qt6::Network
        ZLIB::ZLIB
    COMPILE_DEFINITIONS
        DEFAULT_SUBDL_API_KEY="${DEFAULT_SUBDL_API_KEY}"
        OPEN_SUBTITLES_API_KEY="${OPEN_SUBTITLES_API_KEY}"
)
