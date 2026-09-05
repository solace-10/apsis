#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Test
{

// Runs a compute shader and hands back what it wrote, synchronously.
//
// SGP4ComputePass cannot be driven from a test as it stands. It reaches for the engine's
// global RenderSystem and ResourceSystem, and its readback is a state machine spread over
// several frames because RenderSystem::Update() owns the encoder and the submit. A test
// wants one call that returns numbers, so the sequencing is restated here.
//
// What is deliberately not restated is the data: the input and output structs come from
// render/sgp4_compute_pass.hpp, so a test cannot silently disagree with production about
// the layout it is checking.
//
// Native only, and not because of how the suite is built: the readback blocks on
// ProcessEvents, and there is nothing on the web to block with.
class ComputeHarness
{
public:
    ComputeHarness();
    ~ComputeHarness();

    ComputeHarness(const ComputeHarness&) = delete;
    ComputeHarness& operator=(const ComputeHarness&) = delete;

    // False when this machine has no usable adapter, in which case a case should skip
    // rather than fail. GetUnavailableReason() says which step gave up.
    bool IsAvailable() const { return m_Device != nullptr; }
    const std::string& GetUnavailableReason() const { return m_UnavailableReason; }

    // Compiles a .wgsl file the way the engine does, prelude and all, so that a shader
    // which builds here builds in the application too. Throws with the compiler's own
    // messages, at the file's line numbers, if it does not.
    wgpu::ShaderModule CompileFromFile(const std::filesystem::path& path);

    // One thread per input element, with the dispatch rounded up to whole workgroups
    // exactly as SGP4ComputePass does - so a shader that forgets to guard its tail fails
    // here for the same reason it would fail in the application.
    //
    // Two input buffers rather than one, bound at 0 and 1 with the output at 2, because that is
    // what the pass binds: the coefficients change when the roster does and the times change every
    // frame, so they are uploaded separately.
    template <typename OutputT, typename InputT, typename TimeT>
    std::vector<OutputT> Dispatch(const wgpu::ShaderModule& shaderModule, const char* pEntryPoint,
        const std::vector<InputT>& input, const std::vector<TimeT>& times, uint32_t workgroupSize)
    {
        std::vector<OutputT> output(input.size());
        DispatchRaw(shaderModule, pEntryPoint,
            input.data(), input.size() * sizeof(InputT),
            times.data(), times.size() * sizeof(TimeT),
            output.data(), output.size() * sizeof(OutputT),
            input.size(), workgroupSize);
        return output;
    }

private:
    void DispatchRaw(const wgpu::ShaderModule& shaderModule, const char* pEntryPoint,
        const void* pInput, size_t inputSizeInBytes,
        const void* pTimes, size_t timesSizeInBytes,
        void* pOutput, size_t outputSizeInBytes,
        size_t elementCount, uint32_t workgroupSize);

    // Drives the instance until `done` goes true. Every callback here is registered with
    // AllowProcessEvents, so this is what makes them run. The cap turns a device that has
    // stopped answering into a failed test rather than a hung one.
    void PumpUntil(const bool& done, const char* pWhat);

    void ThrowOnDeviceError(const char* pWhat);

    wgpu::Instance m_Instance;
    wgpu::Adapter m_Adapter;
    wgpu::Device m_Device;

    std::string m_UnavailableReason;

    // Uncaptured errors are asynchronous and belong to no particular call, so they are
    // collected here and raised at the next point a test can attribute them to something.
    std::string m_DeviceErrors;
};

} // namespace WingsOfSteel::Test
