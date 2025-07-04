find_package(LibtorrentRasterbar REQUIRED)

add_library(TorrentDownloader 
    src/TorrentDownloader/TorrentDownloader.h
    src/TorrentDownloader/TorrentDownloader.cpp
    src/TorrentDownloader/Notifier.h
    src/TorrentDownloader/Observer.h
    src/TorrentDownloader/Observer.cpp
)

target_link_libraries(TorrentDownloader
    LibtorrentRasterbar::torrent-rasterbar
)