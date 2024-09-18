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

    for (auto& shaderFile : GetVFS()->List("/shaders"))
    {
        GetResourceSystem()->RequestResource(shaderFile, [this](ResourceSharedPtr pResource) {
            ResourceShaderSharedPtr pResourceShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
            ShaderEditorData data{
                .pResource = pResourceShader,
                .code = pResourceShader->GetShaderCode(),
                .state = ShaderState::Compiled
            };

            m_Shaders[pResourceShader->GetName()] = std::move(data);
        });
    }
}

void ShaderEditor::DrawShaderList()
{
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
            m_Selected = entry.first;
            m_TextEditor.SetText(entry.second.code);
        }
    }
    ImGui::EndChild();

    ImGui::Button(ICON_FA_CIRCLE_PLAY " Compile", ImVec2(shaderListWidth, compileButtonHeight));
    ImGui::EndGroup();
}

} // namespace WingsOfSteel::Pandora