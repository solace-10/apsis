#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"
#include "render/shader_compilation_error.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ResourceShader);
DECLARE_SMART_PTR(ShaderCompilationResult);

using OnShaderCompiledCallback = std::function<void(ShaderCompilationResult*)>;

class ShaderCompilationResult
{
public:
    ShaderCompilationResult(uint32_t id);
    ~ShaderCompilationResult();

    enum class State
    {
        Compiling,
        Success,
        Error
    };

    State GetState() const;
    uint32_t GetCompilationId() const;
    wgpu::ShaderModule GetShaderModule() const;
    inline const std::vector<ShaderCompilationError>& GetErrors() const { return m_Errors; }

    friend class ShaderCompiler;

private:
    uint32_t m_CompilationId{ 0 };
    wgpu::ShaderModule m_ShaderModule;
    State m_State{ State::Compiling };
    OnShaderCompiledCallback m_Callback;
    std::vector<ShaderCompilationError> m_Errors;
};

} // namespace WingsOfSteel