#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/smart_ptr.hpp"
#include "render/shader_compilation_result.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(ResourceShader);
DECLARE_SMART_PTR(ShaderCompilationResult);

using OnShaderCompiledCallback = std::function<void(ShaderCompilationResult*)>;

class ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    void Compile(const std::string& code, OnShaderCompiledCallback callback);
    ShaderCompilationResult* GetShaderCompilationResult(uint32_t id) const;

private:
    std::vector<ShaderCompilationResultUniquePtr> m_Results;
};

} // namespace WingsOfSteel::Pandora