# AGENTS.md

Guidelines for AI coding agents working on the Knight-One project (Orbis repository).

## Build Commands

### Configure (run once or after CMakeLists.txt changes)
```bash
cmake --preset debug-windows   # Windows
cmake --preset debug-linux     # Linux
cmake --preset debug-web       # Web (Emscripten)
```

### Build
```bash
cmake --build build/debug-windows --target game   # Windows
cmake --build build/debug-linux --target game     # Linux
cmake --build build/debug-web --target game       # Web
```

### Build All Targets
```bash
cmake --build build/debug-windows --target setup_compile_commands game
```

## Testing

No test framework. Verify changes by building successfully.

## Linting / Formatting

Format with clang-format (WebKit-based): `clang-format -i path/to/file.cpp`

## Project Structure

- `game/` - Main game application (space exploration)
- `pandora/` - Core engine library (rendering, ECS, physics, resources)

## Code Style Guidelines

### Formatting (.clang-format)
- **Indent**: 4 spaces, no tabs
- **Braces**: Allman style (on new lines)
- **Line length**: No limit
- **Pointer alignment**: Left (`int* ptr`)

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

### Include Order
1. Corresponding header (for .cpp)
2. Standard library (`<vector>`)
3. Third-party (`<imgui.h>`)
4. Engine headers (`<pandora.hpp>`)
5. Local headers (`"sector/sector.hpp"`)

### Header Files
- Use `#pragma once` (no include guards)
- Forward declare where possible

### Smart Pointers
```cpp
DECLARE_SMART_PTR(Sector);  // Generates: SectorSharedPtr, SectorWeakPtr, SectorUniquePtr
SectorSharedPtr m_pSector;
```

### Namespace
```cpp
namespace WingsOfSteel
{
    class MyClass { };
} // namespace WingsOfSteel
```

### Types
- `std::optional<T>` for nullable values
- `enum class` (scoped enums) exclusively
- `std::function<>` for callbacks

### Error Handling
```cpp
// Result type for failable operations
Result<DeserializationError, std::string> result = TryDeserializeString(...);
if (result.has_value()) { return result.value(); }

// Logging
Log::Info() << "Message";
Log::Warning() << "Warning";
Log::Error() << "Error: " << details;
```

### Class Structure
```cpp
#pragma once
#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(MyClass);
class MyClass
{
public:
    MyClass();
    ~MyClass();
    void Initialize();
    
    Sector* GetSector() { return m_pSector.get(); }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    SectorSharedPtr m_pSector;
    bool m_Enabled{ false };
};

} // namespace WingsOfSteel
```

### Platform-Specific Code
```cpp
#if defined(TARGET_PLATFORM_WEB)
#elif defined(TARGET_PLATFORM_WINDOWS)
#elif defined(TARGET_PLATFORM_LINUX)
#endif
```

## Key Patterns

### ECS
- Entities: `Scene::CreateEntity()`
- Components: inherit `IComponent`, use `REGISTER_COMPONENT`
- Systems: inherit `System`, implement `Initialize()` and `Update()`

### Resource Loading (async)
```cpp
GetResourceSystem()->RequestResource("/path/resource.json",
    [](ResourceSharedPtr pResource) { /* handle */ });
```

### Global Accessors
```cpp
GetRenderSystem(), GetInputSystem(), GetResourceSystem(),
GetVFS(), GetWindow(), GetActiveScene(), GetDebugRender()
```

## Important Notes

1. **Always build after changes** to verify compilation
2. **Use absolute Windows paths** for file operations
3. **C++20 standard** - use modern features
4. **No tests** - verify by building and running
