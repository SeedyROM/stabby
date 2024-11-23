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