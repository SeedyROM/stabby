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
    exit /b 1
)

rem Create marker directory
if not exist build\.markers mkdir build\.markers

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
if "%1"=="run-debug" goto run-debug
if "%1"=="run-release" goto run-release
if "%1"=="run" goto run
if "%1"=="run-editor-debug" goto run-editor-debug
if "%1"=="run-editor-release" goto run-editor-release
if "%1"=="run-editor" goto run-editor
if "%1"=="play" goto play
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
echo   build-dev    - Build debug version of game and editor
echo   build-release- Build release version of game and editor
echo   build       - Build debug version (default)
echo   run         - Build and run the game
echo   run-editor  - Build and run the editor
echo   clean       - Remove build directory
echo.
echo Examples:
echo   make deps    - Install dependencies
echo   make build   - Build debug version
echo   make run     - Build and run the game
echo   make run-editor - Build and run the editor
echo   make play    - Build and run the game
echo   make clean   - Remove build directory
goto :eof

:deps-debug
if exist conanfile.txt (
    conan install . --build=missing --profile=profiles/windows.profile -s build_type=RelWithDebInfo
    type nul > build\.markers\deps-relwithdebinfo
) else (
    echo conanfile.txt not found
    exit /b 1
)
goto :eof

:deps-release
if exist conanfile.txt (
    conan install . --build=missing --profile=profiles/windows.profile -s build_type=Release
    type nul > build\.markers\deps-release
) else (
    echo conanfile.txt not found
    exit /b 1
)
goto :eof

:deps
call :deps-debug
goto :eof

:build-dev
if not exist build\.markers\deps-relwithdebinfo call :deps-debug
cmake --preset conan-relwithdebinfo -S . -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
if errorlevel 1 goto :error
type nul > build\.markers\cmake-debug
cmake --build build/RelWithDebInfo --target stabby
cmake --build build/RelWithDebInfo --target editor
if errorlevel 1 goto :error
goto :eof

:build-release
if not exist build\.markers\deps-release call :deps-release
cmake --preset conan-release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :error
type nul > build\.markers\cmake-release
cmake --build build/Release --target stabby
cmake --build build/Release --target editor
if errorlevel 1 goto :error
goto :eof

:build
call :build-dev
goto :eof

:run-debug
call :build-dev
build\RelWithDebInfo\src\game\stabby.exe
goto :eof

:run-release
call :build-release
build\Release\src\game\stabby.exe
goto :eof

:run
call :run-debug
goto :eof

:run-editor-debug
call :build-dev
build\RelWithDebInfo\src\editor\editor.exe
goto :eof

:run-editor-release
call :build-release
build\Release\src\editor\editor.exe
goto :eof

:run-editor
call :run-editor-debug
goto :eof

:play
call :run
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