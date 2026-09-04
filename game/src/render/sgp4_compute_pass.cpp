#include "render/sgp4_compute_pass.hpp"

#include <array>

#include <core/log.hpp>
#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <resources/resource_system.hpp>

namespace WingsOfSteel
{

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
        m_PropagatedPositionsBuffer = nullptr;
        m_BindGroup = nullptr;
        m_pReadback = std::make_shared<Readback>();
        return;
    }

    wgpu::Device& device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor inputBufferDescriptor{
        .label = "SGP4 orbital elements buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = numOrbitalElements * sizeof(OrbitalElementsInput)
    };
    m_OrbitalElementsBuffer = device.CreateBuffer(&inputBufferDescriptor);

    wgpu::BufferDescriptor outputBufferDescriptor{
        .label = "SGP4 propagated positions buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc,
        .size = numOrbitalElements * sizeof(OrbitalElementsOutput)
    };
    m_PropagatedPositionsBuffer = device.CreateBuffer(&outputBufferDescriptor);

    // A fresh Readback rather than a resized one: any map still in flight holds the old
    // object alive and will complete against the old buffer, harmlessly.
    m_pReadback = std::make_shared<Readback>();
    m_pReadback->sizeInBytes = numOrbitalElements * sizeof(OrbitalElementsOutput);

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
    if (!m_OrbitalElementsBuffer || !m_PropagatedPositionsBuffer)
    {
        m_BindGroup = nullptr;
        return;
    }

    // clang-format off
    std::array<wgpu::BindGroupEntry, 2> entries = {
        wgpu::BindGroupEntry{
            .binding = 0,
            .buffer = m_OrbitalElementsBuffer
        },
        wgpu::BindGroupEntry{
            .binding = 1,
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

void SGP4ComputePass::SetOrbitalElements(const std::vector<OrbitalElementsInput>& orbitalElements, EntityRosterSharedPtr pRoster)
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
        m_OrbitalElementsBuffer, 0, orbitalElements.data(), orbitalElements.size() * sizeof(OrbitalElementsInput));
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
        m_pReadback->dispatchTime = std::chrono::system_clock::now();
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
                const OrbitalElementsOutput* pMappedResults = static_cast<const OrbitalElementsOutput*>(
                    buffer.GetConstMappedRange(0, pReadback->sizeInBytes));

                const size_t numResults = pReadback->sizeInBytes / sizeof(OrbitalElementsOutput);
                pResults->positions.resize(numResults);
                for (size_t i = 0; i < numResults; i++)
                {
                    pResults->positions[i] = pMappedResults[i].position;
                }

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
