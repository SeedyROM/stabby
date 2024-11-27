# Check for required dependencies
REQUIRED_CMDS := conan cmake ninja
$(foreach cmd,$(REQUIRED_CMDS),\
    $(if $(shell command -v $(cmd) 2> /dev/null),,\
        $(error Please install '$(cmd)'. \
            conan: pip install conan, \
            cmake: https://cmake.org/download/, \
            ninja: pip install ninja)))

# Directory markers for dependency tracking
MARKER_DIR := ./build/.markers
$(shell mkdir -p $(MARKER_DIR))

# Source file tracking
SRCS := $(shell find src -name '*.cpp' -o -name '*.h' -o -name '*.hpp')
CMAKE_FILES := $(shell find . -name 'CMakeLists.txt')

# Dependency markers
DEPS_DEBUG_MARKER := $(MARKER_DIR)/deps-relwithdebinfo
DEPS_RELEASE_MARKER := $(MARKER_DIR)/deps-release
CMAKE_DEBUG_MARKER := $(MARKER_DIR)/cmake-debug
CMAKE_RELEASE_MARKER := $(MARKER_DIR)/cmake-release

# Executables
DEBUG_EXE := build/RelWithDebInfo/src/game/stabby
RELEASE_EXE := build/Release/src/game/stabby

# Dependencies targets
$(DEPS_DEBUG_MARKER): conanfile.txt
	conan install . --build=missing -s build_type=RelWithDebInfo
	@mkdir -p $(MARKER_DIR)
	@touch $@

$(DEPS_RELEASE_MARKER): conanfile.txt
	conan install . --build=missing -s build_type=Release
	@mkdir -p $(MARKER_DIR)
	@touch $@

deps-debug: $(DEPS_DEBUG_MARKER)
deps-release: $(DEPS_RELEASE_MARKER)
deps: deps-debug

# CMake configuration markers
$(CMAKE_DEBUG_MARKER): $(DEPS_DEBUG_MARKER) $(CMAKE_FILES)
	cmake --preset conan-relwithdebinfo -S . -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@mkdir -p $(MARKER_DIR)
	@touch $@

$(CMAKE_RELEASE_MARKER): $(DEPS_RELEASE_MARKER) $(CMAKE_FILES)
	cmake --preset conan-release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
	@mkdir -p $(MARKER_DIR)
	@touch $@

# Build targets
$(DEBUG_EXE): $(CMAKE_DEBUG_MARKER) $(SRCS)
	cmake --build build/RelWithDebInfo

$(RELEASE_EXE): $(CMAKE_RELEASE_MARKER) $(SRCS)
	cmake --build build/Release

build-dev: $(DEBUG_EXE)
build-release: $(RELEASE_EXE)
build: build-dev

# Run targets
run-debug: $(DEBUG_EXE)
	./$(DEBUG_EXE)

run-release: $(RELEASE_EXE)
	./$(RELEASE_EXE)

run: run-debug

# Clean target
clean:
	rm -rf build $(MARKER_DIR)

play: run

.PHONY: deps-debug deps-release deps build-dev build-release build run-debug run-release run play clean help

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  deps-debug    - Install debug dependencies using Conan"
	@echo "  deps-release  - Install release dependencies using Conan"
	@echo "  deps         - Install debug dependencies (default)"
	@echo "  build-dev    - Build debug version"
	@echo "  build-release- Build release version"
	@echo "  build       - Build debug version (default)"
	@echo "  run         - Build and run the game"
	@echo "  clean       - Remove build directory"
	@echo ""
	@echo "Examples:"
	@echo "  make deps    - Install dependencies"
	@echo "  make build   - Build debug version"
	@echo "  make run     - Build and run the game"
	@echo "  make play   -  Build and run the game"
	@echo "  make clean   - Remove build directory"

.DEFAULT_GOAL := help