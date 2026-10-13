# Vendored KTX-Software

The `texture` asset tool encodes KTX2 files by invoking the `ktx` command line
tool from [KTX-Software](https://github.com/KhronosGroup/KTX-Software). It is
vendored here so that a fresh checkout can compile assets without anyone having
to install anything first.

**Version: 4.4.2** ([release](https://github.com/KhronosGroup/KTX-Software/releases/tag/v4.4.2))

| Directory | Contents | Taken from |
|---|---|---|
| `linux-amd64/` | `ktx`, `libktx.so.4` | `KTX-Software-4.4.2-Linux-x86_64.tar.bz2` |
| `windows-amd64/` | `ktx.exe`, `ktx.dll` | `KTX-Software-4.4.2-Windows-x64.exe` |
| `darwin-arm64/` | `ktx`, `libktx.4.dylib` | `KTX-Software-4.4.2-Darwin-arm64.pkg` |

These are the only encoders Forge will use. It does not fall back to a `ktx`
on `PATH`, so that a texture compiled on one machine is identical to the same
texture compiled on another. Building assets on a platform not listed above
therefore means vendoring its binaries here first.

## Requirements

- **Linux:** glibc 2.34 or newer. The binary's `RUNPATH` is `$ORIGIN`, so
  `libktx.so.4` is found beside it with no environment setup.
- **Windows:** the Visual C++ runtime (`MSVCP140.dll`, `VCRUNTIME140.dll`),
  which is present on any machine with Visual Studio installed.
- **macOS:** 11 or newer, Apple Silicon. The binary's `LC_RPATH` includes
  `@executable_path`, so `libktx.4.dylib` is found beside it with no
  environment setup. Both files keep Khronos's Developer ID signature.

## Adding a platform or updating the version

Download the release for each platform and copy the files listed above into a
directory named `<GOOS>-<GOARCH>`, which is what Forge looks for.
On Linux the library is published as `libktx.so.4.4.2`; rename it to
`libktx.so.4`, which is the SONAME the executable looks for. The Windows
release is an NSIS installer rather than an archive, so extract it with
`7z x KTX-Software-<version>-Windows-x64.exe` and take the files from `bin/`.
The macOS release is a signed installer package; unpack it without installing
via `pkgutil --expand-full KTX-Software-<version>-Darwin-arm64.pkg <dir>`, then
take `ktx` from `*-tools.pkg/Payload/usr/local/bin/` and `libktx.<version>.dylib`
from `*-library.pkg/Payload/usr/local/lib/`, renaming the latter to
`libktx.4.dylib`, which is the install name the executable looks for.

Refresh `licenses/` from the matching tag, and update the version above.

Use **4.3 or newer**. Forge passes `--assign-tf`, which older releases spell
`--assign-oetf`.

## Licensing

KTX-Software is predominantly Apache-2.0. It also incorporates components
under other licenses, all of which are reproduced in `licenses/`.

Note that `libktx` includes Ericsson's ETC codec, which is **not** open source
and is covered by `licenses/LicenseRef-ETCSLA.txt`. That license permits
redistribution in binary form, which is how it is used here: `ktx` is a build
tool that Forge runs as a separate process. It is never linked into, or shipped
with, the game.
