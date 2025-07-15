# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a C++20 CMake project called "The Brightest Star" with multiple executable targets:

### Build Commands
- **Configure**: `cmake --preset debug-linux` (or debug-windows/debug-web)
- **Build**: `cmake --build build/debug-linux --target game` (primary executable)
- **All targets**: `cmake --build build/debug-linux`

### Available Targets
- `game` - Main game executable (primary target)
- `dome` - Geometry processing tool for texture edge detection and mesh generation
- `forge` - Asset manifest management tool
- `pandora` - Core engine library (not an executable)

### Dependencies
Linux setup: `sudo apt install clang clangd cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libglfw3-dev libx11-xcb-dev`

## Architecture

### Core Components
The project follows a modular architecture with four main components:

1. **Pandora** (`pandora/`) - Core engine library providing:
   - Rendering system (WebGPU-based via Dawn)
   - Resource management (textures, models, shaders, fonts)
   - ImGui integration and window management
   - Physics system (Bullet3), input system, VFS
   - ECS-based scene management with Entity/Component system

2. **Game** (`game/`) - Main game application (`WingsOfSteel::TheBrightestStar` namespace):
   - Space exploration game with sectors, ships, hyperscape navigation
   - Ship builder with prefab system and hardpoint management
   - Player controller, weapon systems, navigation systems
   - Fleet management and encounter mechanics

3. **Dome** (`dome/`) - Geometry processing tool (`WingsOfSteel::Dome` namespace):
   - Edge detection from textures, vertex simplification
   - Mesh generation with Delaunay triangulation
   - Texture processing pipeline (greyscale conversion, edge detection)

4. **Forge** (`forge/`) - Asset management tool (`WingsOfSteel::Forge` namespace):
   - Asset manifest creation and management
   - Build pipeline integration for asset processing

### Key Technologies
- **Graphics**: WebGPU (Dawn), OpenGL fallback, ImGui for UI
- **Physics**: Bullet3 for collision detection and physics simulation
- **Math**: GLM for linear algebra operations
- **Serialization**: nlohmann/json for configuration and data files
- **Asset Loading**: tinygltf for 3D model loading, stb libraries for image processing
- **Reflection**: magic_enum for compile-time enum reflection

### Code Conventions
- Namespace pattern: `WingsOfSteel::<ModuleName>`
- Smart pointer macros: `DECLARE_SMART_PTR(ClassName)` generates typedefs
- Modern C++20 features used throughout
- Header-only libraries preferred where possible

### File Organization
- `/src/` contains implementation files
- `/assets/` contains game assets (models, textures, fonts, UI icons)
- `/bin/` contains build outputs and data files
- Third-party dependencies in `pandora/ext/`

## Development Notes
- Compile commands generated automatically for clangd integration
- Cross-platform support (Windows, Linux, Web via Emscripten)
- Uses Ninja generator for builds
- Physics and rendering systems are decoupled from game logic