#include <sstream>
#include <vector>

#include "resources/resource_shader.hpp"

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/shader_compilation_result.hpp"
#include "render/shader_compiler.hpp"
#include "resources/resource_system.hpp"

namespace WingsOfSteel::Pandora
{

ResourceShader::ResourceShader()
{
}

ResourceShader::~ResourceShader()
{
}

void ResourceShader::Load(const std::string& path)
{
    Resource::Load(path);

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceShader::GetResourceType() const
{
    return ResourceType::Shader;
}

wgpu::ShaderModule ResourceShader::GetShaderModule() const
{
    return m_ShaderModule;
}

const std::string& ResourceShader::GetShaderCode() const
{
    return m_ShaderCode;
}

void ResourceShader::Inject(const std::string& code, OnShaderCompiledCallback callback)
{
    const std::string label(GetPath() + " (injected)");
    GetRenderSystem()->GetShaderCompiler()->Compile(label, code, [this, code, callback](ShaderCompilationResult* pCompilationResult) {
        if (pCompilationResult->GetState() == ShaderCompilationResult::State::Success)
        {
            m_ShaderModule = pCompilationResult->GetShaderModule();
            m_ShaderCode = code;

            GetResourceSystem()->GetShaderInjectedSignal().Emit(this);
            Save();
        }

        callback(pCompilationResult);
    });
}

void ResourceShader::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        m_ShaderCode = std::string(pFile->GetData().data(), pFile->GetData().size());

        const std::string label(pFile->GetPath());
        GetRenderSystem()->GetShaderCompiler()->Compile(label, m_ShaderCode, [this, pFile](ShaderCompilationResult* pCompilationResult) {
            if (pCompilationResult->GetState() == ShaderCompilationResult::State::Success)
            {
                m_ShaderModule = pCompilationResult->GetShaderModule();
                SetState(ResourceState::Loaded);
            }
            else
            {
                std::stringstream ss;
                ss << "Failed to compile shader '" << pFile->GetPath() << "':";
                for (const ShaderCompilationError& error : pCompilationResult->GetErrors())
                {
                    ss << std::endl
                       << "Error on line " << (error.GetLineNumber()) << ", char " << error.GetLinePosition() << ": " << error.GetMessage();
                }
                Log::Error() << ss.str();
                SetState(ResourceState::Error);
            }
        });
    }
    else
    {
        SetState(ResourceState::Error);
    }
}

void ResourceShader::Save()
{
    std::vector<uint8_t> data(m_ShaderCode.begin(), m_ShaderCode.end());
    if (GetVFS()->FileWrite(GetPath(), data))
    {
        Log::Info() << "Saved shader '" << GetPath() << "'.";
    }
    else
    {
        Log::Warning() << "Failed to save shader '" << GetPath() << "'.";
    }
}

} // namespace WingsOfSteel::Pandora
