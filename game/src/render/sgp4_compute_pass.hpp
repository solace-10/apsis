#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <render/pass/pass.hpp>
#include <resources/resource_shader.hpp>
#include <scene/entity.hpp>

#include "space/sgp4.hpp"

namespace WingsOfSteel
{

// One satellite's SGP4 coefficients and the time to propagate them to, as uploaded to the GPU.
// Mirrors SGP4ElementSet in sgp4.wgsl - the two must be kept in step.
//
// This is SGP4ElementSet narrowed to f32 and with a time attached. The narrowing is deliberate and
// is the thing being measured: initialisation runs in double on the CPU precisely so that only the
// step has to survive f32, and these are the values it survives with.
//
// The time lives in its own buffer rather than in here: these are fixed for the life of the object
// and are uploaded when the roster changes, where the times change every frame. The coefficient
// names are Vallado's, as they are everywhere else.
struct SGP4StepInput
{
    // Vallado's isimp, and whether SGP4Initialise() declined the element set as deep space. Both
    // are flags rather than bools because WGSL has no bool in host-shareable memory.
    uint32_t simplifiedDrag;
    uint32_t deepSpace;

    float bstar;
    float ecco;
    float inclo;
    float nodeo;
    float argpo;
    float mo;
    float no_unkozai;
    float aycof;
    float con41;
    float cc1;
    float cc4;
    float cc5;
    float d2;
    float d3;
    float d4;
    float delmo;
    float eta;
    float argpdot;
    float omgcof;
    float sinmao;
    float t2cof;
    float t3cof;
    float t4cof;
    float t5cof;
    float x1mth2;
    float x7thm1;
    float mdot;
    float nodedot;
    float xlcof;
    float xmcof;
    float nodecf;

    // SDP4Terms, flattened. Deep-space element sets carry it and near-earth ones leave it zero;
    // it is uploaded either way, because one struct for both branches is what lets the roster be
    // one roster. Names and order are SDP4Terms' own, so the two can be read side by side.
    //
    // resonance mirrors SDP4Resonance rather than being a bool, because there are two arms and
    // they share no arithmetic: 1 is synchronous, 2 is half-day, 0 integrates nothing at all.
    uint32_t resonance;

    // Secular rates from the Sun and Moon, radians per minute.
    float dedt;
    float didt;
    float dmdt;
    float dnodt;
    float domdt;

    // Lunar-solar periodic amplitudes, evaluated afresh each step rather than accumulated.
    float e3;
    float ee2;
    float se2;
    float se3;
    float sgh2;
    float sgh3;
    float sgh4;
    float sh2;
    float sh3;
    float si2;
    float si3;
    float sl2;
    float sl3;
    float sl4;
    float xgh2;
    float xgh3;
    float xgh4;
    float xh2;
    float xh3;
    float xi2;
    float xi3;
    float xl2;
    float xl3;
    float xl4;

    // Where the Moon and Sun were at the epoch.
    float zmol;
    float zmos;

    // Earth resonance terms: the ten d-coefficients for the half-day arm, the three del ones for
    // synchronous. At most one group is ever non-zero.
    float d2201;
    float d2211;
    float d3210;
    float d3222;
    float d4410;
    float d4422;
    float d5220;
    float d5232;
    float d5421;
    float d5433;
    float del1;
    float del2;
    float del3;

    // The sidereal time the resonance is measured against, and what the integration starts from.
    float gsto;
    float xfact;
    float xlamo;

    float padding[3];
};

// Mirrors PropagatedState in sgp4.wgsl. A vec3 aligns to 16 bytes there, so the two trailing
// slots are space the struct would occupy regardless - and the first of them is spent on the
// error, which a shader has no other way to report.
struct SGP4StepOutput
{
    glm::vec3 position; // km, TEME
    uint32_t error; // an SGP4Error
    glm::vec3 velocity; // km/s, TEME
    float padding;
};

static_assert(sizeof(SGP4StepInput) == 336, "SGP4StepInput must match its WGSL counterpart");
static_assert(sizeof(SGP4StepOutput) == 32, "SGP4StepOutput must match its WGSL counterpart");

// Packs an initialised element set for the GPU.
//
// Lives here, beside the struct it fills, so that the shader tests upload through exactly the same
// code the pass does. A test that packed its own inputs could agree with the shader perfectly
// while production disagreed with both.
SGP4StepInput MakeSGP4StepInput(const SGP4ElementSet& elementSet);

// Which entity each element in a dispatch belongs to. Held by shared_ptr and handed to
// the pass alongside the elements, so that a roster stays alive for as long as the
// readback it describes - the results of a dispatch arrive a couple of frames after the
// set of tracked objects may have moved on.
using EntityRoster = std::vector<EntityHandle>;
using EntityRosterSharedPtr = std::shared_ptr<const EntityRoster>;

// The outcome of one completed readback. states[i] belongs to (*pEntities)[i], and the two are
// only ever assigned together, so they cannot fall out of step.
//
// The whole output is kept rather than just the positions: the error says whether a position is
// worth believing at all, and the velocity is already paid for by the time it arrives.
struct PropagationResults
{
    std::vector<SGP4StepOutput> states;
    EntityRosterSharedPtr pEntities;
    std::chrono::system_clock::time_point time; // The instant the states are valid for.
};

// Propagates the position of every tracked space object on the GPU and copies the
// results back for the CPU to consume.
//
// The readback is asynchronous and cannot be made otherwise: mapping only completes once
// the GPU has caught up, and on the web there is no way to block on it at all. Positions
// therefore lag the dispatch that produced them by a couple of frames, which is why the
// results carry both the roster they belong to and the instant they are valid for,
// rather than leaving the caller to assume either.
DECLARE_SMART_PTR(SGP4ComputePass);
class SGP4ComputePass : public Pass
{
public:
    SGP4ComputePass();
    ~SGP4ComputePass();

    void Execute(wgpu::CommandEncoder& encoder) override;

    // False until the shader has loaded and the pipeline exists. SetOrbitalElements() is
    // inert before this point, as the bind group layout comes from the pipeline.
    bool IsReady() const { return m_ComputePipeline != nullptr; }

    // Uploads a new set of objects to propagate. Only needed when the roster changes:
    // orbital elements are static per object, so re-uploading an unchanged set achieves
    // nothing.
    void SetOrbitalElements(const std::vector<SGP4StepInput>& orbitalElements, EntityRosterSharedPtr pRoster);

    // Uploads the time to propagate each object to, in minutes from its own epoch, in roster
    // order. Unlike the coefficients this changes every frame, which is why the two are separate
    // buffers: the times are one float per object against the coefficients' hundred and forty.
    //
    // The instant is carried through to the results rather than re-read when the dispatch is
    // recorded. Those are the same frame but not the same moment, and a sixteen-millisecond frame
    // is a hundred and twenty metres of orbit.
    void SetTimes(const std::vector<float>& tsinceMinutes, std::chrono::system_clock::time_point instant);

    // The most recent completed readback. Empty until the first one lands, and not
    // necessarily describing the roster most recently uploaded.
    const PropagationResults& GetResults() const { return *m_pResults; }

private:
    // Held by shared_ptr and captured by value into the map callback, so that a callback
    // arriving after this pass is destroyed - or after a resize has moved on to a new
    // buffer - writes into an orphan nobody reads rather than into freed memory.
    struct Readback
    {
        enum class State
        {
            Idle, // Nothing in flight; free to record a copy into the buffer.
            CopyRecorded, // Copy recorded, but not submitted until the end of the frame.
            MapPending // Map requested; waiting on the callback.
        };

        State state{ State::Idle };
        wgpu::Buffer buffer;
        size_t sizeInBytes{ 0 };

        // Both describe the copy currently in flight.
        std::chrono::system_clock::time_point dispatchTime;
        EntityRosterSharedPtr pDispatchRoster;
    };

    void CreateComputePipeline();
    void CreateStorageBuffers(size_t numOrbitalElements);
    void CreateBindGroup();
    void RequestReadbackMap();
    void HandleShaderInjection();

    static constexpr uint32_t kWorkgroupSize = 64;

    ResourceShaderSharedPtr m_pShader;
    wgpu::ComputePipeline m_ComputePipeline;
    wgpu::Buffer m_OrbitalElementsBuffer;
    wgpu::Buffer m_TimesBuffer;

    // The instant the times currently in m_TimesBuffer were computed for.
    std::chrono::system_clock::time_point m_TimesInstant;
    wgpu::Buffer m_PropagatedPositionsBuffer;
    wgpu::BindGroup m_BindGroup;
    std::optional<SignalId> m_ShaderInjectionSignalId;
    std::shared_ptr<Readback> m_pReadback;

    // Never replaced, only written to, and captured by every map callback: results
    // outlive the Readback that produced them, and a late callback from an orphaned
    // readback still delivers usable positions because its roster says whose they are.
    std::shared_ptr<PropagationResults> m_pResults;

    EntityRosterSharedPtr m_pRoster;
    size_t m_NumOrbitalElements{ 0 };
};

} // namespace WingsOfSteel
