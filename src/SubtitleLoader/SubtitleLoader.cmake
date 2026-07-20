include(cmake/AddTarget.cmake)

AddTarget(
    TARGET_NAME TorrentPlayerSubtitles
    TYPE STATIC_LIB
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
        glog::glog
    COMPILE_DEFINITIONS
        DEFAULT_SUBDL_API_KEY="${DEFAULT_SUBDL_API_KEY}"
        OPEN_SUBTITLES_API_KEY="${OPEN_SUBTITLES_API_KEY}"
)
