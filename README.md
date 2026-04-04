<div align="center">
<img src=".assets/screenshot.jpg"/>
</div>

# Orbis

> [!WARNING]
> This project is a work in progress and not ready for general use.

<img src="https://healthchecks.io/b/2/40a1ae63-4ec9-41fe-a663-c591b0178e4f.svg" alt="Celestrak">
<img src="https://healthchecks.io/b/2/ff38a4b0-fb05-4708-acae-23e67ab83747.svg" alt="Spacetrack">

## Overview

A space object tracking and visualisation application. **Orbis** renders satellites (e.g. ISS, Starlink, GPS, debris) based on data from [CelesTrak](https://celestrak.org) and [Space-Track](https://www.space-track.org) and performs real-time orbital propagation. 

## Project Structure

- `game/` - Main application (space object logic, rendering, components, systems)
- `pandora/` - [C++20 game engine framework](https://codeberg.org/pedronunes/pandora/) (WebGPU, ECS, physics, resources)
- `webapp/` - SvelteKit frontend for web deployment

## License

**Orbis** is licensed under the GPLv3 License, see [LICENSE](LICENSE) for more information.
