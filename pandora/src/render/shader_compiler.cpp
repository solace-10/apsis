#include <webgpu/webgpu_cpp.h>

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/shader_compiler.hpp"
#include "render/shader_preprocessor.hpp"

namespace WingsOfSteel
{

ShaderCompiler::ShaderCompiler()
{
    ShaderPreprocessor::Initialize();
}

ShaderCompiler::~ShaderCompiler()
{
}

void ShaderCompiler::Compile(const std::string& label, const std::string& code, OnShaderCompiledCallback callback)
{
    const uint32_t id = static_cast<uint32_t>(m_Results.size());
    m_Results.push_back(std::move(std::make_unique<ShaderCompilationResult>(id)));

    wgpu::Device device = GetRenderSystem()->GetDevice();

    std::string preprocessedCode = ShaderPreprocessor::Execute(code);
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = preprocessedCode.c_str();

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
        .nextInChain = &wgslDesc,
        .label = label.data()
    };

    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    GetShaderCompilationResult(id)->m_Callback = callback;
    GetShaderCompilationResult(id)->m_ShaderModule = device.CreateShaderModule(&shaderModuleDescriptor);
    device.PopErrorScope(
        [](WGPUErrorType type, const char* pMessage, void* pUserData) {
            const uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pUserData));
            ShaderCompilationResult* pResult = GetRenderSystem()->GetShaderCompiler()->GetShaderCompilationResult(id);
            if (pResult)
            {
                if (type == WGPUErrorType_NoError)
                {
                    pResult->m_State = ShaderCompilationResult::State::Success;
                    pResult->m_Callback(pResult);
                    pResult->m_Callback = nullptr;
                }
                else
                {
                    pResult->m_State = ShaderCompilationResult::State::Error;

                    pResult->m_ShaderModule.GetCompilationInfo(
                        [](WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* pCompilationInfo, void* pUserData) {
                            const uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pUserData));
                            ShaderCompilationResult* pResult = GetRenderSystem()->GetShaderCompiler()->GetShaderCompilationResult(id);
                            if (pResult)
                            {
                                if (status == WGPUCompilationInfoRequestStatus_Success)
                                {
                                    pResult->m_Errors.reserve(pCompilationInfo->messageCount);
                                    for (size_t i = 0; i < pCompilationInfo->messageCount; i++)
                                    {
                                        const WGPUCompilationMessage& message = pCompilationInfo->messages[i];
                                        pResult->m_Errors.emplace_back(
                                            message.message,
                                            ShaderPreprocessor::ResolveLineNumber(static_cast<uint32_t>(message.lineNum)),
                                            static_cast<uint32_t>(message.linePos));
                                    }
                                }
                                else
                                {
                                    Log::Error() << "WGPUCompilationInfoRequest failed with status code " << status << ".";
                                }
                            }
                            else
                            {
                                Log::Error() << "Failed to find shader for ID " << id << ".";
                            }

                            pResult->m_Callback(pResult);
                            pResult->m_Callback = nullptr;
                        },
                        reinterpret_cast<void*>(static_cast<uintptr_t>(id)));
                }
            }
            else
            {
                Log::Error() << "Failed to find shader for ID " << id << ".";
            }
        },
        reinterpret_cast<void*>(static_cast<uintptr_t>(id)));
}

ShaderCompilationResult* ShaderCompiler::GetShaderCompilationResult(uint32_t id) const
{
    if (id < m_Results.size())
    {
        return m_Results[id].get();
    }
    else
    {
        return nullptr;
    }
}

} // namespace WingsOfSteel