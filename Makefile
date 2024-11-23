# Check for required dependencies
REQUIRED_CMDS := conan cmake ninja
$(foreach cmd,$(REQUIRED_CMDS),\
    $(if $(shell command -v $(cmd) 2> /dev/null),,\
        $(error Please install '$(cmd)'. \
        	conan: pip install conan, \
        	cmake: https://cmake.org/download/, \
        	ninja: pip install ninja)))

deps-debug: conanfile.txt
	conan install . --build=missing -s build_type=Debug

deps-release: conanfile.txt
	conan install . --build=missing -s build_type=Release

deps: deps-debug

build-dev: 
	cmake --preset conan-debug -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build build/Debug

build-release:
	cmake --preset conan-release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build build/Release

build: build-dev

run-debug: build-dev
	./build/Debug/src/game/stabby

run-release: build-release
	./build/Release/src/game/stabby

run: run-debug

clean:
	rm -rf build

play: run

.PHONY: deps-debug deps-release deps build-dev build-release build run play clean

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