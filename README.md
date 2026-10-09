<div align="center">
<img src=".assets/screenshot.jpg"/>
</div>

# Apsis.earth

> [!WARNING]
> This project is a work in progress and not ready for general use.

<img src="https://healthchecks.io/b/2/40a1ae63-4ec9-41fe-a663-c591b0178e4f.svg" alt="Celestrak">
<img src="https://healthchecks.io/b/2/ff38a4b0-fb05-4708-acae-23e67ab83747.svg" alt="Spacetrack">

## Overview

A space object tracking and visualisation application. **Apsis** renders satellites (e.g. ISS, Starlink, GPS, debris) based on data from [CelesTrak](https://celestrak.org) and [Space-Track](https://www.space-track.org) and performs real-time orbital propagation. 

## Project Structure

- `game/` - Main application (space object logic, rendering, components, systems)
- `pandora/` - [C++20 game engine framework](https://github.com/solace-10/pandora) (WebGPU, ECS, physics, resources)
- `webapp/` - SvelteKit frontend for web deployment

## Building

### Prerequisites

CMake 3.24+, Ninja, a C++20 compiler and Emscripten SDK.

### First-time setup

```bash
git submodule update --init pandora
./pandora/scripts/setup_emscripten.sh
```

Web builds also read `FORGE_AUTH_KEY_SECRET` from a `.env` file at the repository root; it is
gitignored and not required for native builds.

### Building

```bash
./build.sh <platform> <build_type>   # platform: linux|windows|web, build_type: debug|release
```

Builds are output to `build/<build_type>-<platform>/game/`.

### Webapp

```bash
cd webapp
npm install
npm run dev     # development server
```

### Tests

```bash
./scripts/test.sh              # everything
./scripts/test.sh -R earth_frame   # ctest arguments are passed through
```

The suite is native-only and always built Debug.

## Credits

- [Space-Track](https://www.space-track.org): authoritative source for orbital data.
- [CelesTrack](https://celestrak.org): my thanks to Professor T. S. Kelso for grouping the orbital data and making it accessible to the public.
- Madoc Glasssmith: for his assistance with setting up this project's database and authoring SQL queries.

## License

**Apsis** is licensed under the GPLv3 License, see [LICENSE](LICENSE) for more information.
