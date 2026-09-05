#include "gpu/compute_harness.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <magic_enum.hpp>

#include <render/shader_preprocessor.hpp>

namespace WingsOfSteel::Test
{

namespace
{

    // Long enough that a busy GPU is never mistaken for a wedged one, short enough that a
    // wedged one is not mistaken for a slow test run.
    constexpr int kMaxPumpIterations = 100000;

    // Buffer sizes and copy lengths have to be multiples of four. The structs in play are
    // already 16 and 32 bytes, but rounding here means the harness stays honest about that
    // rather than depending on it.
    size_t RoundUpToFour(size_t size)
    {
        return (size + 3u) & ~static_cast<size_t>(3u);
    }

    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("Failed to open shader " + path.string());
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

} // namespace

ComputeHarness::ComputeHarness()
{
    m_Instance = wgpu::CreateInstance();
    if (!m_Instance)
    {
        m_UnavailableReason = "WebGPU instance could not be created.";
        return;
    }

    // Deliberately not RenderSystem::AcquireDevice(). That function is private, and it
    // calls exit(-1) when there is no adapter - which on a machine without a GPU would take
    // the whole ctest run down instead of skipping the handful of cases that need one. It
    // is worth noting that it needs no window either: it runs in full before glfwInit().
    bool adapterDone = false;
    m_Instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::AllowProcessEvents,
        [this, &adapterDone](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success)
            {
                m_Adapter = std::move(adapter);
            }
            else
            {
                m_UnavailableReason = "No WebGPU adapter: " + std::string(std::string_view(message));
            }
            adapterDone = true;
        });
    PumpUntil(adapterDone, "adapter request");

    if (!m_Adapter)
    {
        return;
    }

    // No required features. The compressed texture formats RenderSystem asks for are for
    // loading KTX2 textures and have nothing to say about compute.
    wgpu::DeviceDescriptor deviceDescriptor{};
    // A lost device would otherwise show up as every subsequent case failing for its own
    // inscrutable reason, so it is recorded in the same place uncaptured errors are.
    deviceDescriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowProcessEvents,
        [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message, ComputeHarness* pHarness) {
            if (reason != wgpu::DeviceLostReason::Destroyed)
            {
                pHarness->m_DeviceErrors += "device lost (" + std::string(magic_enum::enum_name(reason)) + "): " + std::string(std::string_view(message)) + "\n";
            }
        },
        this);

    // Dawn will not take a capturing lambda here, so the harness arrives as userdata. The
    // errors are collected rather than acted on: an uncaptured error belongs to no
    // particular call, and the point is to attach it to the next one a test can name.
    deviceDescriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message, ComputeHarness* pHarness) {
            pHarness->m_DeviceErrors += std::string(magic_enum::enum_name(type)) + ": " + std::string(std::string_view(message)) + "\n";
        },
        this);

    bool deviceDone = false;
    m_Adapter.RequestDevice(
        &deviceDescriptor, wgpu::CallbackMode::AllowProcessEvents,
        [this, &deviceDone](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success)
            {
                m_Device = std::move(device);
            }
            else
            {
                m_UnavailableReason = "No WebGPU device: " + std::string(std::string_view(message));
            }
            deviceDone = true;
        });
    PumpUntil(deviceDone, "device request");
}

ComputeHarness::~ComputeHarness()
{
    m_Device = nullptr;
    m_Adapter = nullptr;
    m_Instance = nullptr;
}

void ComputeHarness::PumpUntil(const bool& done, const char* pWhat)
{
    for (int i = 0; i < kMaxPumpIterations && !done; i++)
    {
        m_Instance.ProcessEvents();
    }

    if (!done)
    {
        throw std::runtime_error(std::string("WebGPU ") + pWhat + " never completed.");
    }
}

void ComputeHarness::ThrowOnDeviceError(const char* pWhat)
{
    if (m_DeviceErrors.empty())
    {
        return;
    }

    const std::string errors = std::move(m_DeviceErrors);
    m_DeviceErrors.clear();
    throw std::runtime_error(std::string("WebGPU device error during ") + pWhat + ":\n" + errors);
}

wgpu::ShaderModule ComputeHarness::CompileFromFile(const std::filesystem::path& path)
{
    // The same prelude the engine prepends, taken from the engine rather than restated, so
    // that a shader relying on something the prelude declares cannot compile in one place
    // and not the other.
    ShaderPreprocessor::Initialize();
    const std::string source = ShaderPreprocessor::Execute(ReadFile(path));

    wgpu::ShaderSourceWGSL wgslDescriptor{};
    wgslDescriptor.code = source.c_str();

    const std::string label = path.filename().string();
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
        .nextInChain = &wgslDescriptor,
        .label = label.c_str()
    };

    m_Device.PushErrorScope(wgpu::ErrorFilter::Validation);
    wgpu::ShaderModule shaderModule = m_Device.CreateShaderModule(&shaderModuleDescriptor);

    bool failed = false;
    bool scopeDone = false;
    m_Device.PopErrorScope(
        wgpu::CallbackMode::AllowProcessEvents,
        [&failed, &scopeDone](wgpu::PopErrorScopeStatus, wgpu::ErrorType type, wgpu::StringView) {
            failed = (type != wgpu::ErrorType::NoError);
            scopeDone = true;
        });
    PumpUntil(scopeDone, "shader compilation");

    if (!failed)
    {
        return shaderModule;
    }

    // Line numbers come back counting the prelude, so they are put back into the file's own
    // terms the way the shader editor does - an error reported against a line the file does
    // not have is worse than no line number at all.
    std::ostringstream errors;
    errors << "Failed to compile " << path.string() << ":";

    bool infoDone = false;
    shaderModule.GetCompilationInfo(
        wgpu::CallbackMode::AllowProcessEvents,
        [&errors, &infoDone](wgpu::CompilationInfoRequestStatus status, const wgpu::CompilationInfo* pCompilationInfo) {
            if (status == wgpu::CompilationInfoRequestStatus::Success)
            {
                for (size_t i = 0; i < pCompilationInfo->messageCount; i++)
                {
                    const wgpu::CompilationMessage& message = pCompilationInfo->messages[i];
                    errors << "\n  line " << ShaderPreprocessor::ResolveLineNumber(static_cast<uint32_t>(message.lineNum))
                           << ": " << std::string_view(message.message);
                }
            }
            infoDone = true;
        });
    PumpUntil(infoDone, "shader compilation info");

    throw std::runtime_error(errors.str());
}

void ComputeHarness::DispatchRaw(const wgpu::ShaderModule& shaderModule, const char* pEntryPoint,
    const void* pInput, size_t inputSizeInBytes,
    const void* pTimes, size_t timesSizeInBytes,
    void* pOutput, size_t outputSizeInBytes,
    size_t elementCount, uint32_t workgroupSize)
{
    if (!IsAvailable())
    {
        throw std::runtime_error("Dispatch on a harness with no device: " + m_UnavailableReason);
    }

    if (inputSizeInBytes == 0 || outputSizeInBytes == 0)
    {
        throw std::runtime_error("Dispatch with nothing to do.");
    }

    const size_t inputBufferSize = RoundUpToFour(inputSizeInBytes);
    const size_t outputBufferSize = RoundUpToFour(outputSizeInBytes);

    wgpu::ComputePipelineDescriptor computePipelineDescriptor{
        .label = "Test compute pipeline",
        .compute = { .module = shaderModule, .entryPoint = pEntryPoint }
    };
    wgpu::ComputePipeline computePipeline = m_Device.CreateComputePipeline(&computePipelineDescriptor);

    wgpu::BufferDescriptor inputBufferDescriptor{
        .label = "Test compute input buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = inputBufferSize
    };
    wgpu::Buffer inputBuffer = m_Device.CreateBuffer(&inputBufferDescriptor);

    wgpu::BufferDescriptor timesBufferDescriptor{
        .label = "Test compute times buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = timesSizeInBytes
    };
    wgpu::Buffer timesBuffer = m_Device.CreateBuffer(&timesBufferDescriptor);

    wgpu::BufferDescriptor outputBufferDescriptor{
        .label = "Test compute output buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc,
        .size = outputBufferSize
    };
    wgpu::Buffer outputBuffer = m_Device.CreateBuffer(&outputBufferDescriptor);

    // MapRead may only be paired with CopyDst, which is why the results are staged through
    // a second buffer here just as they are in the compute pass.
    wgpu::BufferDescriptor readbackBufferDescriptor{
        .label = "Test compute readback buffer",
        .usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst,
        .size = outputBufferSize
    };
    wgpu::Buffer readbackBuffer = m_Device.CreateBuffer(&readbackBufferDescriptor);

    m_Device.GetQueue().WriteBuffer(inputBuffer, 0, pInput, inputSizeInBytes);
    m_Device.GetQueue().WriteBuffer(timesBuffer, 0, pTimes, timesSizeInBytes);

    // clang-format off
    std::array<wgpu::BindGroupEntry, 3> entries = {
        wgpu::BindGroupEntry{
            .binding = 0,
            .buffer = inputBuffer
        },
        wgpu::BindGroupEntry{
            .binding = 1,
            .buffer = timesBuffer
        },
        wgpu::BindGroupEntry{
            .binding = 2,
            .buffer = outputBuffer
        }
    };
    // clang-format on

    // The layout comes from the pipeline rather than being declared here, so the test goes
    // through the same reflection the application does and would notice a shader whose
    // bindings had drifted from what the pass binds.
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .label = "Test compute bind group",
        .layout = computePipeline.GetBindGroupLayout(0),
        .entryCount = entries.size(),
        .entries = entries.data()
    };
    wgpu::BindGroup bindGroup = m_Device.CreateBindGroup(&bindGroupDescriptor);

    wgpu::CommandEncoderDescriptor commandEncoderDescriptor{
        .label = "Test compute command encoder"
    };
    wgpu::CommandEncoder encoder = m_Device.CreateCommandEncoder(&commandEncoderDescriptor);

    wgpu::ComputePassDescriptor computePassDescriptor{
        .label = "Test compute pass"
    };
    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDescriptor);
    computePass.SetPipeline(computePipeline);
    computePass.SetBindGroup(0, bindGroup);
    computePass.DispatchWorkgroups(static_cast<uint32_t>((elementCount + workgroupSize - 1) / workgroupSize));
    computePass.End();

    encoder.CopyBufferToBuffer(outputBuffer, 0, readbackBuffer, 0, outputBufferSize);

    wgpu::CommandBufferDescriptor commandBufferDescriptor{
        .label = "Test compute command buffer"
    };
    wgpu::CommandBuffer commands = encoder.Finish(&commandBufferDescriptor);
    m_Device.GetQueue().Submit(1, &commands);

    bool mapDone = false;
    wgpu::MapAsyncStatus mapStatus = wgpu::MapAsyncStatus::Error;
    std::string mapMessage;
    readbackBuffer.MapAsync(
        wgpu::MapMode::Read, 0, outputBufferSize, wgpu::CallbackMode::AllowProcessEvents,
        [&mapDone, &mapStatus, &mapMessage](wgpu::MapAsyncStatus status, wgpu::StringView message) {
            mapStatus = status;
            mapMessage = std::string(std::string_view(message));
            mapDone = true;
        });
    PumpUntil(mapDone, "buffer readback");

    ThrowOnDeviceError("dispatch");

    if (mapStatus != wgpu::MapAsyncStatus::Success)
    {
        throw std::runtime_error("Buffer readback failed: " + mapMessage);
    }

    std::memcpy(pOutput, readbackBuffer.GetConstMappedRange(0, outputBufferSize), outputSizeInBytes);
    readbackBuffer.Unmap();
}

} // namespace WingsOfSteel::Test
