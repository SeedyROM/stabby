@echo off
setlocal enabledelayedexpansion

if "%1"=="" goto default
if "%1"=="deps-debug" goto deps-debug
if "%1"=="deps-release" goto deps-release
if "%1"=="deps" goto deps
if "%1"=="build-dev" goto build-dev
if "%1"=="build-release" goto build-release
if "%1"=="build" goto build
if "%1"=="run" goto run
if "%1"=="clean" goto clean
echo Invalid target
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

:default
call :build
goto :eof

:error
echo Build failed
exit /b 1