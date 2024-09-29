#include "render/shader_compilation_result.hpp"

namespace WingsOfSteel::Pandora
{

ShaderCompilationResult::ShaderCompilationResult(uint32_t id)
: m_CompilationId(id)
{

}

ShaderCompilationResult::~ShaderCompilationResult()
{

}

ShaderCompilationResult::State ShaderCompilationResult::GetState() const
{
    return m_State;
}

uint32_t ShaderCompilationResult::GetCompilationId() const
{
    return m_CompilationId;
}

wgpu::ShaderModule ShaderCompilationResult::GetShaderModule() const
{
    return m_ShaderModule;
}

} // namespace WingsOfSteel::Pandora