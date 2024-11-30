# Stabby 🔪

A modern 2D hack 'n' slash game built with a custom C++ game engine. This project reimagines a previous XNA/FNA/Monogame prototype with platform independence and no Visual Studio build tool dependencies.

## Project Status 🚀

### Completed

- [x] Separate engine from game code via CMake
- [x] Setup Conan
- [x] Initial project setup with Conan and SDL2/GLAD/GLM
- [x] Create reproducible builds with `Makefile`/`make.bat` script code
- [x] Import/translate modified ECS from C# prototype
- [x] Create simple sprite renderer using OpenGL
- [x] Refactor/move C# ECS implementation into C++23
- [x] Resource loading
- [x] Audio system basics
- [x] Dynamic audio playback system with sound effects, music and speed control

### In Progress

- [ ] Immediate mode UI basics
- [ ] Initial gameplay implementation
- [ ] Game-specific features

## Engine Features

The game is built on a custom C++ engine (STE - Stabby Engine) designed for high-performance 2D/3D(?) games.

### Core Systems

#### Entity Component System (ECS)

- Modern implementation using C++20 concepts
- Sparse set component storage for efficient iteration
- Type-safe component queries
- Systems with priority scheduling
- Resource management

#### Asset Management

- Asynchronous asset loading
- Reference counting
- Support for textures and shaders
- Thread-safe asset handling
- Error handling with detailed feedback

#### Rendering

- 2D batch renderer
- Triple buffered command processing
- Shader management
- Texture support
- Efficient sprite rendering

#### Audio System

- Real-time audio processing
- Low-latency output
- Support for multiple audio channels
- Sample-accurate timing
- Thread-safe command queue

#### Window Management

- Modern builder pattern API
- Support for multiple windows
- OpenGL context management
- Event handling
- Resolution and display mode control

## Technical Implementation

### Dependencies

- SDL2 - Window management and input
- OpenGL - Rendering
- GLM - Mathematics
- GLAD - OpenGL loading
- Conan - Package management

### Architecture Highlights

- Modern C++20/23 features
- RAII throughout the codebase
- Lock-free concurrent operations
- Efficient batch rendering
- Thread pool for async operations
- Comprehensive error handling

## Building the Project

### Prerequisites

- CMake
- Conan
- C++20 compatible compiler

### Build Steps

See the [build instructions](BUILD.md) for detailed information on how to build the project.

## Example Usage

See the game entrypoint used for testing [here](src/game/main.cpp).
