find_package(LibtorrentRasterbar REQUIRED)

add_library(TorrentDownloader 
    src/TorrentDownloader/TorrentDownloader.h
    src/TorrentDownloader/TorrentDownloader.cpp
)

target_link_libraries(TorrentDownloader
    LibtorrentRasterbar::torrent-rasterbar
)