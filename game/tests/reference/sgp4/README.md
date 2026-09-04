# Vallado's SGP4, vendored for tests

`SGP4.cpp` and `SGP4.h` are the reference implementation of SGP4/SDP4 that every other
implementation is measured against: the companion code to *Revisiting Spacetrack Report #3*
(Vallado, Crawford, Hujsak and Kelso, AIAA 2006-6753). They are here unmodified, so that the
tests have something to compare the compute shader against that is not another piece of our
own arithmetic.

## Provenance

Taken from the copy in [python-sgp4](https://github.com/brandon-rhodes/python-sgp4), which
carries the 2023-05-09 release from Vallado's *Fundamentals of Astrodynamics and
Applications* software page verbatim:

| File | Source | SHA256 |
|---|---|---|
| `SGP4.cpp` | `raw.githubusercontent.com/brandon-rhodes/python-sgp4/master/extension/SGP4.cpp` | `2ee7ad0e8f201e8251894083fe21e33a7aace2f43c871bf04357eb44a891b06e` |
| `SGP4.h` | `raw.githubusercontent.com/brandon-rhodes/python-sgp4/master/extension/SGP4.h` | `2a5ec44e059a52b3173d78d9a28bda8142b4f6497c1eb16febc2cea4b5006b0c` |
| `../../data/SGP4-VER.TLE` | `.../master/sgp4/SGP4-VER.TLE` | `d246d1d9d768ace445a38a965713fa9ba52d80fd8a41a0502ff83d7acffe2881` |
| `../../data/tcppver.out` | `.../master/sgp4/tcppver.out` | `687bf28dbe52df86e8e60ab5cb4a08d1aa3dbcaf4e63b1f7ab95f044fbe3833b` |

The last two are the published verification set: 33 element sets chosen to exercise every
branch of the algorithm, and the position and velocity Vallado's own build produces for each.
`space/sgp4_tests.cpp` reproduces that output, which is what makes this copy trustworthy
rather than merely present. It agrees to within the precision the output file is printed at,
except for the 3.5-year propagation of 20413 where it reaches 1.2e-7 km.

The files ship with CRLF line endings and are left that way; `sgp4_reference.cpp` strips the
carriage returns when reading the data, and `.clang-format-ignore` keeps clang-format off the
source, both so that this stays diffable against upstream.

## Licence

The source banner names its authors and the AIAA paper but states no licence. CelesTrak's own
mirror of the same code, [fundamentals-of-astrodynamics](https://github.com/CelesTrak/fundamentals-of-astrodynamics),
is labelled AGPL-3.0; python-sgp4 redistributes these same files under MIT. The position is
genuinely unclear, and the answer here is to keep the question from arising: `sgp4_reference`
is its own CMake target, linked only by `tests`, never by `game_lib`, and so never present in
anything that ships. If that ever stops being true, this needs settling first.
