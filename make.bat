@echo off
setlocal enabledelayedexpansion

rem Check required dependencies
where conan >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Conan is not installed or not in PATH
    echo Install with: pip install conan
    exit /b 1
)

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: CMake is not installed or not in PATH
    echo Download from: https://cmake.org/download/
    exit /b 1
)

where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Ninja is not installed or not in PATH
    echo Install with: pip install ninja
    echo Or download from: https://github.com/ninja-build/ninja/releases
    exit /b 1
)

if "%1"=="" goto usage
if "%1"=="-h" goto usage
if "%1"=="--help" goto usage
if "%1"=="help" goto usage
if "%1"=="deps-debug" goto deps-debug
if "%1"=="deps-release" goto deps-release
if "%1"=="deps" goto deps
if "%1"=="build-dev" goto build-dev
if "%1"=="build-release" goto build-release
if "%1"=="build" goto build
if "%1"=="run" goto run
if "%1"=="clean" goto clean
echo Invalid target
goto usage

:usage
echo Usage: make [target]
echo.
echo Targets:
echo   deps-debug    - Install debug dependencies using Conan
echo   deps-release  - Install release dependencies using Conan
echo   deps         - Install debug dependencies (default)
echo   build-dev    - Build debug version
echo   build-release- Build release version
echo   build       - Build debug version (default)
echo   run         - Build and run the game
echo   clean       - Remove build directory
echo.
echo Examples:
echo   make deps    - Install dependencies
echo   make build   - Build debug version
echo   make run     - Build and run the game
goto :eof

:deps-debug
if exist conanfile.txt (
    conan install . --output-folder=build --build=missing -s build_type=Debug
) else (
    echo conanfile.txt not found
    exit /b 1
)
goto :eof

:deps-release
if exist conanfile.txt (
    conan install . --output-folder=build --build=missing -s build_type=Release
) else (
    echo conanfile.txt not found
    exit /b 1
)
goto :eof

:deps
call :deps-debug
goto :eof

:build-dev
cmake --preset conan-debug -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto :error
cmake --build build
if errorlevel 1 goto :error
goto :eof

:build-release
cmake --preset conan-release -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :error
cmake --build build
if errorlevel 1 goto :error
goto :eof

:build
call :build-dev
goto :eof

:run
call :build
build\src\game\stabby.exe
goto :eof

:clean
if exist build (
    rmdir /s /q build
    echo Build directory cleaned
) else (
    echo Build directory does not exist
)
goto :eof

:error
echo Build failed
exit /b 1