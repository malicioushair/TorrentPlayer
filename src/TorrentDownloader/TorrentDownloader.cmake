include(cmake/AddTarget.cmake)

AddTarget(
    TARGET_NAME TorrentDownloader
    TYPE SHARED_LIB

    PACKAGE LibtorrentRasterbar
    PACKAGE glog

    DEPENDENCIES
        LibtorrentRasterbar::torrent-rasterbar
        glog::glog
)
