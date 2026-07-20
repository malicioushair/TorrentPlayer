from conan import ConanFile
from conan.tools.cmake import CMakeDeps


class TorrentPlayerConan(ConanFile):
    name = "torrentplayer"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("libtorrent/2.0.9")
        self.requires("glog/0.7.1")
        self.requires("keychain/1.3.0")
        self.requires("gtest/1.14.0")
        self.requires("zlib/1.3.1")

    def configure(self):
        self.options["glog"].shared = True
        self.options["gflags"].shared = True

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
