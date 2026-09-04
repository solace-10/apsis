// Mirrors OrbitalElementsInput in game/src/render/sgp4_compute_pass.hpp.
// The two must be kept in step; a static_assert there pins the size.
struct OrbitalElements
{
    meanMotion: f32,
    eccentricity: f32,
    inclination: f32,
    raan: f32,
    argumentOfPericenter: f32,
    meanAnomaly: f32,
    padding0: f32,
    padding1: f32,
}

// Mirrors OrbitalElementsOutput. A vec3f aligns to 16 bytes, so the trailing float
// occupies padding that the struct would carry regardless.
struct PropagatedPosition
{
    position: vec3f,
    padding: f32,
}

@group(0) @binding(0) var<storage, read> orbitalElements: array<OrbitalElements>;
@group(0) @binding(1) var<storage, read_write> propagatedPositions: array<PropagatedPosition>;

@compute @workgroup_size(64) fn computeSGP4(
    @builtin(global_invocation_id) id: vec3u
) {
    let i = id.x;

    // The dispatch is rounded up to whole workgroups, so the tail runs past the end.
    if (i >= arrayLength(&orbitalElements)) {
        return;
    }

    let elements = orbitalElements[i];

    // TODO: the SGP4 propagator itself. Until it lands this only echoes three of the
    // inputs back, so that a readback can be checked against known element values
    // without the result depending on any maths that is not written yet.
    propagatedPositions[i].position = vec3f(elements.meanMotion, elements.eccentricity, elements.inclination);
}
