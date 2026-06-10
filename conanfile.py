import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.cmake import CMake
from conan.errors import ConanInvalidConfiguration


class KunstkammerRecipe(ConanFile):
    name = "kunstkammer"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("boost/1.91.0")
        self.requires("openssl/3.6.2")
        self.requires("gtest/1.17.0")
        self.requires("spdlog/1.17.0")

    def build_requirements(self):
        self.tool_requires("cmake/4.3.2")

    def layout(self):
        cmake_layout(self)
        # # We make the assumption that if the compiler is msvc the
        # # CMake generator is multi-config
        # multi = True if self.settings.get_safe("compiler") == "msvc" else False
        # if multi:
        #     self.folders.generators = os.path.join("build", "generators")
        #     self.folders.build = "build"
        # else:
        #     self.folders.generators = os.path.join("build", str(self.settings.build_type), "generators")
        #     self.folders.build = os.path.join("build", str(self.settings.build_type))

    def validate(self):
        if self.settings.os == "Macos" and self.settings.arch == "armv8":
            raise ConanInvalidConfiguration("ARM v8 not supported in Macos")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()    