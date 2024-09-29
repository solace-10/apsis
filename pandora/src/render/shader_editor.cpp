#include "render/shader_editor.hpp"

#include "imgui/fonts/icons_font_awesome.hpp"
#include "resources/resource_shader.hpp"
#include "resources/resource_system.hpp"
#include "vfs/vfs.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ShaderEditor::ShaderEditor()
{
}
    
ShaderEditor::~ShaderEditor()
{

}

void ShaderEditor::Update()
{
    if (!m_Show)
    {
        return;
    }

    if (!m_Initialized)
    {
        Initialize();
    }

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Once);
    if (ImGui::Begin("Shader editor", &m_Show))
    {
        DrawShaderList();
        ImGui::SameLine();


        
        m_TextEditor.Render("Shader editor", ImVec2(), true);
        ImGui::End();
    }
}

void ShaderEditor::Show(bool state)
{
    m_Show = state;
}

void ShaderEditor::Initialize()
{
    m_Initialized = true;

    auto shadersList = GetVFS()->List("/shaders");
    m_ShadersToLoad = shadersList.size();
    for (auto& shaderFile : shadersList)
    {
        GetResourceSystem()->RequestResource(shaderFile, [this](ResourceSharedPtr pResource) {
            ResourceShaderSharedPtr pResourceShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
            ShaderEditorData data{
                .pResource = pResourceShader,
                .code = pResourceShader->GetShaderCode(),
                .state = ShaderState::Compiled
            };

            m_Shaders[pResourceShader->GetName()] = std::move(data);
            m_ShadersToLoad--;
        });
    }
}

void ShaderEditor::DrawShaderList()
{
    // Open the first shader once all the shaders have been loaded.
    if (m_Selected.empty() && !m_Shaders.empty() && m_ShadersToLoad == 0)
    {
        OpenShader(m_Shaders.begin()->first);
    }

    const int shaderListWidth = 250;
    const int compileButtonHeight = 32;
    ImGui::BeginGroup();
    ImGui::BeginChild("ShaderList", ImVec2(shaderListWidth, ImGui::GetContentRegionAvail().y - compileButtonHeight - 4), ImGuiChildFlags_Border);

    for (auto& entry : m_Shaders)
    {
        bool selected = (entry.first == m_Selected);
        std::string label = ICON_FA_FILE_CODE " " + entry.first;
        if (ImGui::Selectable(label.c_str(), &selected))
        {
            if (m_Selected != entry.first)
            {
                OpenShader(entry.first);
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button(ICON_FA_CIRCLE_PLAY " Compile", ImVec2(shaderListWidth, compileButtonHeight)))
    {
        CompileSelectedShader();
    }

    ImGui::EndGroup();
}

void ShaderEditor::OpenShader(const std::string& shader)
{
    m_Selected = shader;
    m_TextEditor.SetText(m_Shaders[shader].code);
    m_TextEditor.SetCursorPosition(TextEditor::Coordinates(0, 0));
}

void ShaderEditor::CompileSelectedShader()
{
    if (m_Selected.empty())
    {
        return;
    }
}

} // namespace WingsOfSteel::Pandora