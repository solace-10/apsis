# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Apsis (Wings of Steel) is a cross-platform space object tracking and visualization application. It renders satellite orbits (ISS, Starlink, GPS, debris) with interactive controls. The project consists of a C++20 game engine (Pandora), the main application (game), a web frontend (webapp), and a Python data ingestion pipeline.

## Build Commands

### First-time setup (per worktree — do this automatically before the first build)

We develop in git worktrees under `~/.herdr/worktrees/apsis/<branch>`. A fresh worktree is
missing several gitignored build prerequisites that are NOT carried across from `main`.
**Before the first build in a worktree, run `./scripts/worktree-setup.sh` without asking.**
It is idempotent (creates only what's absent, borrowing from the canonical checkout at
`~/dev/apsis`) and safe to re-run at any time. It handles:

1. **`pandora/ext/`** (emsdk + FetchContent deps, ~4.1 GB, gitignored via `ext/` in
   `pandora/.gitignore`) — symlinked to the canonical checkout's populated `ext/` to avoid a
   4 GB re-download. Consequence: it is *shared*, so a dependency bump or
   `pandora/scripts/setup_emscripten.sh` run in either tree is seen by both.
2. **`.env`** at the repo root (holds `FORGE_AUTH_KEY_SECRET` for `forge upload`, gitignored) —
   copied from the canonical checkout.
3. **`game/bin/manifest.json`** (gitignored, generated per checkout) — bootstrapped via
   `./pandora/tools/forge/bin/forge-<os>-<arch> manifest`. This is not strictly required,
   because the `add_custom_command` in `game/CMakeLists.txt` declares
   `OUTPUT "${MANIFEST_OUTPUT}"` and CMake can therefore generate it itself.

After this, `./build.sh linux debug` works.

Note that `forge manifest` ships **everything** under `game/bin/data/core/` and has no exclude
mechanism, so don't put documentation or other non-runtime files in that tree — see
`CREDITS.md` at the repo root.

### Pandora changes (git subtree)

`pandora/` is a git subtree of `git@github.com:solace-10/pandora.git`, which other projects
share. It is plain files in this repo: edit and commit engine and game changes together, in one
commit if that's natural, on any branch. Sync with upstream via `./scripts/pandora.sh`:
- `status` — engine commits not yet pushed upstream, and upstream commits not yet pulled.
- `sync` — `pull` (squash-merge upstream `main` into `pandora/`), then `push` (split the
  `pandora/` part of our commits out and push it to upstream `main`). Run after `git commit`,
  then `git push` this repo last, since the pull may add a merge commit.
- Only push from this repo's `main`: engine changes on a feature branch go upstream after the
  branch merges, so unlanded experiments don't reach the other projects. The script enforces it.
- Don't run `git subtree` by hand with a custom `-m` on `add`, and don't rewrite the
  `Add pandora as a git subtree` commit: its `git-subtree-*` trailers stop `split` from treating
  early history (when `pandora/` was a plain directory) as pandora's.

### Preferred build entry point
```bash
./build.sh <platform> <build_type>   # platform: linux|windows|web, build_type: debug|release
```

### Configure manually (run once or after CMakeLists.txt changes)
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

Catch2 + CTest, native-only. Run with `./scripts/test.sh` (accepts ctest arguments, e.g.
`./scripts/test.sh -R earth_frame`). The suite is always built Debug, because `PANDORA_ASSERT`
compiles out under NDEBUG.

Tests live in `game/tests/` and link `game_lib` — `game/` is built as a static library plus a
thin `main.cpp` executable, so anything in the game is reachable from a test without maintaining
a second list of which sources are testable. `game/tests/NOTES.md` records behaviour the tests
deliberately pin, and the known gaps.

Note that `game` links `game_lib` with `WHOLE_ARCHIVE`: a translation unit that exists only for a
static initialiser (`src/emscripten/bindings.cpp`) is otherwise dropped by the linker, silently.

Not everything is covered — building successfully and running the application is still the check
for rendering and engine-entangled changes.

## Formatting

Format with clang-format (WebKit-based): `clang-format -i path/to/file.cpp`

## Project Structure

- `game/` - Main application (components, systems, rendering, space object logic)
- `pandora/` - Core engine library (rendering, ECS, physics, resources, input, VFS)
- `webapp/` - SvelteKit + TypeScript frontend for web deployment
- `data_ingestion/` - Python scripts for fetching space object data (Celestrak, SpaceTrack) into PostgreSQL, runs in Docker

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
