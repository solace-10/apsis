# Asset credits

Kept at the repo root rather than beside the assets themselves: `forge manifest` ships
everything under `game/bin/data/core/` to R2, and it has no exclude mechanism, so a docs file
in that tree would be uploaded as a runtime asset.

Sources live in `game/assets/textures/`; Forge compiles each one to the KTX2 named in its
`*.asset.json` descriptor.

## game/assets/textures/8k_earth_color_map.jpg

Earth diffuse (colour) map, 8192x4096 equirectangular. Compiled to
`textures/earth_color.ktx2`.

- **Source**: NASA Earth Observatory, *Blue Marble: Next Generation*, April 2004, base set.
- **Original file**: `world.200404.3x21600x10800.jpg` (21600x10800), from the `bmng-base`
  collection on <https://assets.science.nasa.gov/>, same URL pattern as the
  topography/bathymetry set previously used here.
- **Credit**: NASA Earth Observatory. Imagery by Reto Stockli, based on data from the MODIS
  instrument aboard NASA's Terra satellite, with ocean colour and compositing by Robert Simmon.
- **Licence**: Public domain (NASA-produced). Credit is requested but not required.

April was chosen to match the rest of the map set. NASA publishes all twelve months of 2004 at
the same URL pattern if a different season is ever wanted.

### Why the base set rather than topography/bathymetry

This repo previously used the topography-and-bathymetry variant, because `planet.wgsl` sampled
the colour texture directly as `baseColor` with no water term, and the base map's near-black
oceans looked broken.

That reasoning no longer applies. With a specular map available as a land/sea mask, ocean
appearance is computed rather than baked, and the topo/bathy variant actively fights it:

- Bathymetric tint is a depth colour ramp, not photography. Blue light in clear ocean
  attenuates at roughly 0.03/m, so nothing below about 50 m is visible from orbit, and
  mid-ocean ridges at 2000-3000 m certainly are not. The variant paints them anyway.
- Baked ocean colour double-counts against a Fresnel/specular water term.
- The variant's shaded relief is lit from a fixed sun azimuth, which contradicts a moving
  light and shades the same ridgelines twice once the normal map is applied.

The one real loss is genuinely shallow water - the Bahamas banks, the Great Barrier Reef, coral
atolls - which is visibly turquoise from orbit and which a binary land/sea mask will not
reproduce. If it is wanted back, a shallow-water tint driven by the specular map's soft edge is
the cheap approach, not a full bathymetry layer.

### Processing applied

1. Downsampled 21600x10800 -> 8192x4096 with a Lanczos filter, performed in linear light
   (`-colorspace RGB` before resize, back to `-colorspace sRGB` after) so the average brightness
   of coastlines and open ocean is not skewed by resampling in gamma space. A 16-bit
   intermediate (`-depth 16`) is used because the oceans in the base set are dark enough to band
   at 8 bits through the linear round trip.
2. Encoded as JPEG quality 95 with **4:4:4 chroma** (no subsampling). Land/ocean boundaries are
   exactly the high-contrast, high-saturation edges that subsampling handles worst, and the file
   is re-encoded by Forge afterwards, so generation loss is kept low here.
3. Metadata stripped.

Result: 6.6 MB, 46.8 dB PSNR against the lossless downscale. A lossless PNG of the same image is
19.1 MB.

8192x4096 is the largest that fits WebGPU's default `maxTextureDimension2D` of 8192. Forge does
not perform this resize: `ktx create` refuses any input larger than 16384 px, so the 21600 px
original cannot be resampled in-pipeline and is reduced before it enters `game/assets`.

## game/assets/textures/8k_earth_clouds.jpg, 8k_earth_nightmap.jpg, 8k_earth_normal_map.png, 8k_earth_specular_map.png

Cloud, night lights, normal and specular maps, all 8192x4096 equirectangular and aligned with
the colour map above. Compiled to `textures/earth_clouds.ktx2`, `earth_night.ktx2`,
`earth_normal.ktx2` and `earth_specular.ktx2`.

- **Source**: believed to be Solar System Scope's Earth texture pack
  (<https://www.solarsystemscope.com/textures/>), whose distribution filenames these match
  exactly, including the `.tif` extension on the normal and specular maps.
- **Credit**: Solar System Scope. Their Earth maps are themselves derived from NASA elevation
  and imagery data.
- **Licence**: CC BY 4.0.

> **Unverified.** The files carry no authorship metadata - only an Adobe Photoshop CreatorTool
> tag and an sRGB profile - so the attribution above rests on the filenames alone. Confirm the
> origin before shipping publicly, and correct this section if it came from somewhere else.

### Processing applied

The normal and specular maps were converted from TIFF to PNG, because Forge's `texture` tool
only accepts `.png`, `.jpg` and `.jpeg` and `ktx create` cannot decode TIFF. The conversion is
bit-identical (`magick -strip PNG24:`, verified at zero differing pixels). The cloud and night
maps are used as distributed.

### Channel usage

Worth knowing before these are wired up, because Forge compiles all of them to RGBA8:

- **Specular** is fully greyscale (R=G=B) and reads as a clean land/sea mask, 67% ocean.
- **Clouds** is also fully greyscale - one channel of coverage in four.
- **Normal** has its blue channel pinned to a constant 255, so Z carries no information and must
  be reconstructed as `sqrt(1 - x*x - y*y)` if a unit normal is needed. It is flat over water,
  confirming it is land relief only.

Forge has no option to emit R8 or RG8, and `SelectTranscodeFormat` in `ktx2_loader.cpp` only
ever transcodes to BC7, ASTC 4x4, ETC2 RGBA or RGBA32, so three of these ship at four times the
channels they use.
