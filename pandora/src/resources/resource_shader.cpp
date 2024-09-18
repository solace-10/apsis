#include "resources/resource_shader.hpp"

#include "core/log.hpp"
#include "render/rendersystem.hpp"
#include "pandora.hpp"

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
        [this](FileReadResult result, FileSharedPtr pFile)
        {
            this->LoadInternal(result, pFile);
        }
    );
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

bool ResourceShader::SetShaderCode(const std::string& code)
{
    // Not implemented.
    return false;
}

void ResourceShader::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        m_ShaderCode = std::string(pFile->GetData().data(), pFile->GetData().size());

        wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
        wgslDesc.code = m_ShaderCode.c_str();

        wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
            .nextInChain = &wgslDesc
        };
        m_ShaderModule = GetRenderSystem()->GetDevice().CreateShaderModule(&shaderModuleDescriptor);
        //m_ShaderModule.GetCompilationInfo()

        SetState(ResourceState::Loaded);
    }
    else
    {
        SetState(ResourceState::Error);   
    }
}

} // namespace WingsOfSteel::Pandora
