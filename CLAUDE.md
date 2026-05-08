# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Orbis (Wings of Steel) is a cross-platform space object tracking and visualization application. It renders satellite orbits (ISS, Starlink, GPS, debris) with interactive controls. The project consists of a C++20 game engine (Pandora), the main application (game), and a web frontend (webapp).

## Build Commands

### Configure (run once or after CMakeLists.txt changes)
```bash
cmake --preset debug-linux     # Linux
cmake --preset debug-windows   # Windows
cmake --preset debug-web       # Web (Emscripten)
```

### Build
```bash
cmake --build build/debug-linux --target game     # Linux
cmake --build build/debug-windows --target game   # Windows
cmake --build build/debug-web --target game       # Web
```

### Webapp (SvelteKit)
```bash
cd webapp
npm install
npm run dev          # Development server
npm run build        # Production build
```

## Testing

No test framework. Verify changes by building successfully and running the application.

## Formatting

Format with clang-format (WebKit-based): `clang-format -i path/to/file.cpp`

## Project Structure

- `game/` - Main application (components, systems, rendering, space object logic)
- `pandora/` - Core engine library (rendering, ECS, physics, resources, input, VFS)
- `webapp/` - SvelteKit + TypeScript frontend for web deployment

## Code Style

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `Sector`, `CameraSystem` |
| Functions/Methods | PascalCase | `Initialize()`, `GetSector()` |
| Member variables | `m_` prefix | `m_ShowDebug` |
| Member pointers | `m_p` prefix | `m_pCamera` |
| Static variables | `s` prefix | `sShaderMap` |
| Global pointers | `g_p` prefix | `g_pGame` |
| Constants | `k` prefix | `kEarthRadius` |

### Header Files
- Use `#pragma once`
- Forward declare where possible
- Include order: corresponding header → standard library → third-party → engine → local

### Smart Pointers
```cpp
DECLARE_SMART_PTR(Sector);  // Generates: SectorSharedPtr, SectorWeakPtr, SectorUniquePtr
```

### Namespace
```cpp
namespace WingsOfSteel
{
    class MyClass { };
} // namespace WingsOfSteel
```

### Error Handling
```cpp
Result<DeserializationError, std::string> result = TryDeserializeString(...);
if (result.has_value()) { return result.value(); }

Log::Info() << "Message";
Log::Warning() << "Warning";
Log::Error() << "Error: " << details;
```

### Platform-Specific Code
```cpp
#if defined(TARGET_PLATFORM_WEB)
#elif defined(TARGET_PLATFORM_WINDOWS)
#elif defined(TARGET_PLATFORM_LINUX)
#endif
```

## Architecture

### ECS (EnTT-based)
- Entities: `Scene::CreateEntity()`
- Components: inherit `IComponent`, use `REGISTER_COMPONENT` macro
- Systems: inherit `System`, implement `Initialize()` and `Update(float delta)`

### Resource Loading (async)
```cpp
GetResourceSystem()->RequestResource("/path/resource.json",
    [](ResourceSharedPtr pResource) { /* callback */ });
```

### Global Accessors
```cpp
GetRenderSystem(), GetInputSystem(), GetResourceSystem(),
GetVFS(), GetWindow(), GetActiveScene(), GetDebugRender()
```

### Graphics
WebGPU-based rendering. Native builds use Dawn; web builds use browser WebGPU via Emscripten.
