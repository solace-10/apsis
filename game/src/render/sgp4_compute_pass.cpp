#include "render/sgp4_compute_pass.hpp"

#include <array>

#include <core/log.hpp>
#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <resources/resource_system.hpp>

namespace WingsOfSteel
{

SGP4StepInput MakeSGP4StepInput(const SGP4ElementSet& elementSet)
{
    // clang-format off
    return SGP4StepInput{
        .simplifiedDrag = elementSet.simplifiedDrag ? 1u : 0u,
        .deepSpace = elementSet.method == SGP4Method::DeepSpace ? 1u : 0u,
        .bstar = static_cast<float>(elementSet.bstar),
        .ecco = static_cast<float>(elementSet.ecco),
        .inclo = static_cast<float>(elementSet.inclo),
        .nodeo = static_cast<float>(elementSet.nodeo),
        .argpo = static_cast<float>(elementSet.argpo),
        .mo = static_cast<float>(elementSet.mo),
        .no_unkozai = static_cast<float>(elementSet.no_unkozai),
        .aycof = static_cast<float>(elementSet.aycof),
        .con41 = static_cast<float>(elementSet.con41),
        .cc1 = static_cast<float>(elementSet.cc1),
        .cc4 = static_cast<float>(elementSet.cc4),
        .cc5 = static_cast<float>(elementSet.cc5),
        .d2 = static_cast<float>(elementSet.d2),
        .d3 = static_cast<float>(elementSet.d3),
        .d4 = static_cast<float>(elementSet.d4),
        .delmo = static_cast<float>(elementSet.delmo),
        .eta = static_cast<float>(elementSet.eta),
        .argpdot = static_cast<float>(elementSet.argpdot),
        .omgcof = static_cast<float>(elementSet.omgcof),
        .sinmao = static_cast<float>(elementSet.sinmao),
        .t2cof = static_cast<float>(elementSet.t2cof),
        .t3cof = static_cast<float>(elementSet.t3cof),
        .t4cof = static_cast<float>(elementSet.t4cof),
        .t5cof = static_cast<float>(elementSet.t5cof),
        .x1mth2 = static_cast<float>(elementSet.x1mth2),
        .x7thm1 = static_cast<float>(elementSet.x7thm1),
        .mdot = static_cast<float>(elementSet.mdot),
        .nodedot = static_cast<float>(elementSet.nodedot),
        .xlcof = static_cast<float>(elementSet.xlcof),
        .xmcof = static_cast<float>(elementSet.xmcof),
        .nodecf = static_cast<float>(elementSet.nodecf),
        .resonance = static_cast<uint32_t>(elementSet.deepSpace.resonance),
        .dedt = static_cast<float>(elementSet.deepSpace.dedt),
        .didt = static_cast<float>(elementSet.deepSpace.didt),
        .dmdt = static_cast<float>(elementSet.deepSpace.dmdt),
        .dnodt = static_cast<float>(elementSet.deepSpace.dnodt),
        .domdt = static_cast<float>(elementSet.deepSpace.domdt),
        .e3 = static_cast<float>(elementSet.deepSpace.e3),
        .ee2 = static_cast<float>(elementSet.deepSpace.ee2),
        .se2 = static_cast<float>(elementSet.deepSpace.se2),
        .se3 = static_cast<float>(elementSet.deepSpace.se3),
        .sgh2 = static_cast<float>(elementSet.deepSpace.sgh2),
        .sgh3 = static_cast<float>(elementSet.deepSpace.sgh3),
        .sgh4 = static_cast<float>(elementSet.deepSpace.sgh4),
        .sh2 = static_cast<float>(elementSet.deepSpace.sh2),
        .sh3 = static_cast<float>(elementSet.deepSpace.sh3),
        .si2 = static_cast<float>(elementSet.deepSpace.si2),
        .si3 = static_cast<float>(elementSet.deepSpace.si3),
        .sl2 = static_cast<float>(elementSet.deepSpace.sl2),
        .sl3 = static_cast<float>(elementSet.deepSpace.sl3),
        .sl4 = static_cast<float>(elementSet.deepSpace.sl4),
        .xgh2 = static_cast<float>(elementSet.deepSpace.xgh2),
        .xgh3 = static_cast<float>(elementSet.deepSpace.xgh3),
        .xgh4 = static_cast<float>(elementSet.deepSpace.xgh4),
        .xh2 = static_cast<float>(elementSet.deepSpace.xh2),
        .xh3 = static_cast<float>(elementSet.deepSpace.xh3),
        .xi2 = static_cast<float>(elementSet.deepSpace.xi2),
        .xi3 = static_cast<float>(elementSet.deepSpace.xi3),
        .xl2 = static_cast<float>(elementSet.deepSpace.xl2),
        .xl3 = static_cast<float>(elementSet.deepSpace.xl3),
        .xl4 = static_cast<float>(elementSet.deepSpace.xl4),
        .zmol = static_cast<float>(elementSet.deepSpace.zmol),
        .zmos = static_cast<float>(elementSet.deepSpace.zmos),
        .d2201 = static_cast<float>(elementSet.deepSpace.d2201),
        .d2211 = static_cast<float>(elementSet.deepSpace.d2211),
        .d3210 = static_cast<float>(elementSet.deepSpace.d3210),
        .d3222 = static_cast<float>(elementSet.deepSpace.d3222),
        .d4410 = static_cast<float>(elementSet.deepSpace.d4410),
        .d4422 = static_cast<float>(elementSet.deepSpace.d4422),
        .d5220 = static_cast<float>(elementSet.deepSpace.d5220),
        .d5232 = static_cast<float>(elementSet.deepSpace.d5232),
        .d5421 = static_cast<float>(elementSet.deepSpace.d5421),
        .d5433 = static_cast<float>(elementSet.deepSpace.d5433),
        .del1 = static_cast<float>(elementSet.deepSpace.del1),
        .del2 = static_cast<float>(elementSet.deepSpace.del2),
        .del3 = static_cast<float>(elementSet.deepSpace.del3),
        .gsto = static_cast<float>(elementSet.deepSpace.gsto),
        .xfact = static_cast<float>(elementSet.deepSpace.xfact),
        .xlamo = static_cast<float>(elementSet.deepSpace.xlamo)
    };
    // clang-format on
}

SGP4ComputePass::SGP4ComputePass()
    : Pass("SGP4 compute pass")
{
    m_pReadback = std::make_shared<Readback>();
    m_pResults = std::make_shared<PropagationResults>();

    GetResourceSystem()->RequestResource("/shaders/sgp4.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateComputePipeline();
        HandleShaderInjection();
    });
}

SGP4ComputePass::~SGP4ComputePass()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void SGP4ComputePass::CreateComputePipeline()
{
    if (!m_pShader)
    {
        Log::Error() << "Trying to create a compute pipeline without the shader being loaded.";
        return;
    }

    // No layout is given, which is the C++ spelling of "auto": Dawn reflects the shader
    // and generates the bind group layout, retrieved below via GetBindGroupLayout(0).
    wgpu::ComputePipelineDescriptor computePipelineDescriptor{
        .label = "SGP4 compute pipeline",
        .compute = { .module = m_pShader->GetShaderModule(), .entryPoint = "computeSGP4" }
    };

    m_ComputePipeline = GetRenderSystem()->GetDevice().CreateComputePipeline(&computePipelineDescriptor);
}

void SGP4ComputePass::CreateStorageBuffers(size_t numOrbitalElements)
{
    if (numOrbitalElements == 0)
    {
        m_OrbitalElementsBuffer = nullptr;
        m_TimesBuffer = nullptr;
        m_PropagatedPositionsBuffer = nullptr;
        m_BindGroup = nullptr;
        m_pReadback = std::make_shared<Readback>();
        return;
    }

    wgpu::Device& device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor inputBufferDescriptor{
        .label = "SGP4 orbital elements buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = numOrbitalElements * sizeof(SGP4StepInput)
    };
    m_OrbitalElementsBuffer = device.CreateBuffer(&inputBufferDescriptor);

    wgpu::BufferDescriptor timesBufferDescriptor{
        .label = "SGP4 times buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = numOrbitalElements * sizeof(float)
    };
    m_TimesBuffer = device.CreateBuffer(&timesBufferDescriptor);

    wgpu::BufferDescriptor outputBufferDescriptor{
        .label = "SGP4 propagated positions buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc,
        .size = numOrbitalElements * sizeof(SGP4StepOutput)
    };
    m_PropagatedPositionsBuffer = device.CreateBuffer(&outputBufferDescriptor);

    // A fresh Readback rather than a resized one: any map still in flight holds the old
    // object alive and will complete against the old buffer, harmlessly.
    m_pReadback = std::make_shared<Readback>();
    m_pReadback->sizeInBytes = numOrbitalElements * sizeof(SGP4StepOutput);

    // MapRead may only be paired with CopyDst, which is why the results cannot be read
    // out of the storage buffer directly and need this staging copy.
    wgpu::BufferDescriptor readbackBufferDescriptor{
        .label = "SGP4 propagated positions readback buffer",
        .usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst,
        .size = m_pReadback->sizeInBytes
    };
    m_pReadback->buffer = device.CreateBuffer(&readbackBufferDescriptor);

    CreateBindGroup();
}

// Separate from the buffers it binds, because the two are rebuilt for different reasons: the
// buffers when the roster changes size, the bind group additionally whenever the pipeline is
// replaced. Its layout comes from the pipeline, so a bind group outliving the pipeline it was
// built against is a bind group built against a layout that no longer exists.
void SGP4ComputePass::CreateBindGroup()
{
    if (!m_OrbitalElementsBuffer || !m_TimesBuffer || !m_PropagatedPositionsBuffer)
    {
        m_BindGroup = nullptr;
        return;
    }

    // clang-format off
    std::array<wgpu::BindGroupEntry, 3> entries = {
        wgpu::BindGroupEntry{
            .binding = 0,
            .buffer = m_OrbitalElementsBuffer
        },
        wgpu::BindGroupEntry{
            .binding = 1,
            .buffer = m_TimesBuffer
        },
        wgpu::BindGroupEntry{
            .binding = 2,
            .buffer = m_PropagatedPositionsBuffer
        }
    };
    // clang-format on

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .label = "SGP4 bind group",
        .layout = m_ComputePipeline.GetBindGroupLayout(0),
        .entryCount = entries.size(),
        .entries = entries.data()
    };
    m_BindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void SGP4ComputePass::SetOrbitalElements(const std::vector<SGP4StepInput>& orbitalElements, EntityRosterSharedPtr pRoster)
{
    // The bind group layout comes from the pipeline, so there is nothing to build until
    // the shader has loaded.
    if (!IsReady())
    {
        return;
    }

    // The number of orbital elements is not known when the application initializes, so
    // the buffers are built the first time they become available and rebuilt whenever
    // the count changes. The pipeline is unaffected: buffer sizes are not baked into it.
    if (orbitalElements.size() != m_NumOrbitalElements)
    {
        CreateStorageBuffers(orbitalElements.size());
        m_NumOrbitalElements = orbitalElements.size();
    }

    m_pRoster = std::move(pRoster);

    if (m_NumOrbitalElements == 0)
    {
        return;
    }

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(
        m_OrbitalElementsBuffer, 0, orbitalElements.data(), orbitalElements.size() * sizeof(SGP4StepInput));
}

void SGP4ComputePass::SetTimes(const std::vector<float>& tsinceMinutes, std::chrono::system_clock::time_point instant)
{
    // A count that disagrees with the uploaded roster would silently propagate objects to each
    // other's times, so it is refused rather than clamped.
    if (!IsReady() || m_NumOrbitalElements == 0 || tsinceMinutes.size() != m_NumOrbitalElements)
    {
        return;
    }

    m_TimesInstant = instant;

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(
        m_TimesBuffer, 0, tsinceMinutes.data(), tsinceMinutes.size() * sizeof(float));
}

void SGP4ComputePass::Execute(wgpu::CommandEncoder& encoder)
{
    if (!IsReady() || m_NumOrbitalElements == 0)
    {
        return;
    }

    // The copy recorded last frame has been submitted by now, as the RenderSystem
    // submits one command buffer at the end of every frame. Mapping any earlier would
    // wait only on work the queue had already seen, which would not include that copy.
    if (m_pReadback->state == Readback::State::CopyRecorded)
    {
        RequestReadbackMap();
    }

    wgpu::ComputePassDescriptor computePassDescriptor{
        .label = "SGP4 compute pass"
    };

    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDescriptor);
    computePass.SetPipeline(m_ComputePipeline);
    computePass.SetBindGroup(0, m_BindGroup);
    computePass.DispatchWorkgroups(static_cast<uint32_t>((m_NumOrbitalElements + kWorkgroupSize - 1) / kWorkgroupSize));
    computePass.End();

    // CopyBufferToBuffer is an encoder-level command and cannot be recorded inside the
    // compute pass above. It is skipped entirely while a readback is in flight, as a
    // buffer cannot be written to while mapped; dropping a frame's results costs
    // nothing given they are consumed a frame or two late in any case.
    if (m_pReadback->state == Readback::State::Idle)
    {
        encoder.CopyBufferToBuffer(m_PropagatedPositionsBuffer, 0, m_pReadback->buffer, 0, m_pReadback->sizeInBytes);
        m_pReadback->dispatchTime = m_TimesInstant;
        m_pReadback->pDispatchRoster = m_pRoster;
        m_pReadback->state = Readback::State::CopyRecorded;
    }
}

void SGP4ComputePass::RequestReadbackMap()
{
    // Both captures are by value and independently keep their target alive, so the
    // callback remains safe if this pass is destroyed before it fires.
    std::shared_ptr<Readback> pReadback = m_pReadback;
    std::shared_ptr<PropagationResults> pResults = m_pResults;
    wgpu::Buffer buffer = m_pReadback->buffer;

    m_pReadback->state = Readback::State::MapPending;

    buffer.MapAsync(
        wgpu::MapMode::Read, 0, pReadback->sizeInBytes, wgpu::CallbackMode::AllowSpontaneous,
        [pReadback, pResults, buffer](wgpu::MapAsyncStatus status, wgpu::StringView message) {
            if (status == wgpu::MapAsyncStatus::Success)
            {
                const SGP4StepOutput* pMappedResults = static_cast<const SGP4StepOutput*>(
                    buffer.GetConstMappedRange(0, pReadback->sizeInBytes));

                const size_t numResults = pReadback->sizeInBytes / sizeof(SGP4StepOutput);
                pResults->states.assign(pMappedResults, pMappedResults + numResults);

                // Assigned together with the positions, so a consumer can never pair
                // them with the wrong roster.
                pResults->pEntities = pReadback->pDispatchRoster;
                pResults->time = pReadback->dispatchTime;
                buffer.Unmap();
            }
            // A cancelled callback means the device is going away, in which case the
            // buffer must not be touched at all.
            else if (status != wgpu::MapAsyncStatus::CallbackCancelled)
            {
                Log::Error() << "SGP4 readback failed: " << std::string_view(message.data, message.length);
            }

            pReadback->state = Readback::State::Idle;
        });
}

void SGP4ComputePass::HandleShaderInjection()
{
    if (m_ShaderInjectionSignalId.has_value())
    {
        return;
    }

    m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
        [this](ResourceShader* pResourceShader) {
            if (m_pShader.get() != pResourceShader)
            {
                return;
            }

            CreateComputePipeline();
            CreateBindGroup();
        });
}

} // namespace WingsOfSteel
