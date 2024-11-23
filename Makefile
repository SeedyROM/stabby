install-deps-debug: conanfile.txt
	conan install . --output-folder=build --build=missing -s build_type=Debug

install-deps-release: conanfile.txt
	conan install . --output-folder=build --build=missing -s build_type=Release

build-dev: 
	cmake --preset conan-debug -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake  --build build

build-release:
	cmake --preset conan-release -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake  --build build 

build: build-dev

run: build
	./build/src/game/stabby