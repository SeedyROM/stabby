# Check for required dependencies
REQUIRED_CMDS := conan cmake ninja
$(foreach cmd,$(REQUIRED_CMDS),\
    $(if $(shell command -v $(cmd) 2> /dev/null),,\
        $(error Please install '$(cmd)'. \
        	conan: pip install conan, \
        	cmake: https://cmake.org/download/, \
        	ninja: pip install ninja)))

deps-debug: conanfile.txt
	conan install . --output-folder=build --build=missing -s build_type=Debug

deps-release: conanfile.txt
	conan install . --output-folder=build --build=missing -s build_type=Release

deps: deps-debug

build-dev: 
	cmake --preset conan-debug -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake  --build build

build-release:
	cmake --preset conan-release -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake  --build build 

build: build-dev

run: build
	./build/src/game/stabby

clean:
	rm -rf build

.PHONY: deps-debug deps-release deps build-dev build-release build run clean

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

.DEFAULT_GOAL := help