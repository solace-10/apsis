# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Orbis (Wings of Steel) is a cross-platform space object tracking and visualization application. It renders satellite orbits (ISS, Starlink, GPS, debris) with interactive controls. The project consists of a C++20 game engine (Pandora), the main application (game), a web frontend (webapp), and a Python data ingestion pipeline.

## Build Commands

### First-time setup (per worktree — do this automatically before the first build)

We develop in git worktrees under `~/.herdr/worktrees/orbis/<branch>`. A fresh worktree is
missing several gitignored build prerequisites that are NOT carried across from `main`.
**Before the first build in a worktree, run `./scripts/worktree-setup.sh` without asking.**
It is idempotent (creates only what's absent, borrowing from the canonical checkout at
`~/dev/orbis`) and safe to re-run at any time — also useful later to re-attach pandora's HEAD
after a `submodule update` detaches it. It handles:

1. **Engine submodule** — `git submodule update --init pandora`
   (URL: `git@codeberg.org:pedronunes/pandora.git`), then attaches the detached HEAD to `main`
   (tracking `origin/main`) so engine commits aren't dangling. Objects are cached in the shared
   `.git/modules/pandora`, so this is local and fast.
2. **`pandora/ext/`** (emsdk + FetchContent deps, ~4.1 GB, gitignored via `ext/` in
   `pandora/.gitignore`) — symlinked to the canonical checkout's populated `ext/` to avoid a
   4 GB re-download. Consequence: it is *shared*, so a dependency bump or
   `pandora/scripts/setup_emscripten.sh` run in either tree is seen by both.
3. **`.env`** at the repo root (holds `FORGE_AUTH_KEY_SECRET` for `forge upload`, gitignored) —
   copied from the canonical checkout.
4. **`game/bin/manifest.json`** (gitignored, generated per checkout) — bootstrapped via
   `./pandora/tools/forge/bin/forge manifest`. Unlike the sibling `danus_garden` repo this is
   not strictly required, because Orbis's `add_custom_command` in `game/CMakeLists.txt`
   declares `OUTPUT "${MANIFEST_OUTPUT}"` and CMake can therefore generate it itself; the
   script does it anyway to keep both repos' setup behaviour identical.
5. **Git config guardrails** (shared by all worktrees of this repo):
   `push.recurseSubmodules=on-demand` and `submodule.recurse=true`.

After this, `./build.sh linux debug` works.

The script is deliberately kept in step with `danus_garden/scripts/worktree-setup.sh` — both
repos share the pandora engine and the same worktree layout, so the setup behaviour should not
drift between them.

Note that `forge manifest` ships **everything** under `game/bin/data/core/` and has no exclude
mechanism, so don't put documentation or other non-runtime files in that tree — see
`CREDITS.md` at the repo root.

### Committing pandora changes (submodule)

pandora's `main` tracks `origin/main`. To land engine changes:
- Confirm pandora is on `main`, not detached (`git -C pandora status`) *before* committing —
  a commit made while detached is dangling.
- Commit in `pandora/`, then bump the pointer in the outer repo: `git add pandora && git commit`.
- Push from the outer repo; `push.recurseSubmodules=on-demand` pushes pandora first. If pushing
  pandora manually instead, push it *before* the outer repo, or other checkouts can't fetch the
  referenced SHA.
- If two worktree branches both change pandora, land one before starting or rebasing the other —
  parallel engine work on the shared `main` races on push and produces gitlink conflicts.

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
