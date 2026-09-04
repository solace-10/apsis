# Credits

## Assets

### game/assets/textures/8k_earth_color_map.jpg

Earth diffuse (colour) map, 8192x4096 equirectangular.

- **Source**: NASA Earth Observatory, *Blue Marble: Next Generation*, April 2004, base set.
- **Original file**: `world.200404.3x21600x10800.jpg` (21600x10800), from the `bmng-base`
  collection on <https://assets.science.nasa.gov/>.
- **Credit**: NASA Earth Observatory. Imagery by Reto Stockli, based on data from the MODIS
  instrument aboard NASA's Terra satellite, with ocean colour and compositing by Robert Simmon.
- **Licence**: Public domain (NASA-produced). Credit is requested but not required.

### game/assets/textures/8k_earth_clouds.jpg, 8k_earth_nightmap.jpg, 8k_earth_normal_map.png, 8k_earth_specular_map.png

Cloud, night lights, normal and specular maps, all 8192x4096 equirectangular and aligned with
the colour map above.

- **Source**: Solar System Scope's Earth texture pack
  (<https://www.solarsystemscope.com/textures/>), whose distribution filenames these match
  exactly.
- **Credit**: Solar System Scope. Their Earth maps are themselves derived from NASA elevation
  and imagery data.
- **Licence**: CC BY 4.0 (Attribution 4.0 International).

## Test-only third-party code

### game/tests/reference/sgp4/

David Vallado's reference SGP4 implementation and its published verification data, used by
the test suite to check our own propagation against. Linked only by the `tests` target and
never by `game_lib`, so none of it reaches a shipped binary. Provenance, checksums and the
licence position are recorded in `game/tests/reference/sgp4/README.md`.
