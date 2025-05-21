#include "render/shader_editor.hpp"

#include "imgui/fonts/icons_font_awesome.hpp"
#include "imgui/imgui_system.hpp"
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

        if (!m_Selected.empty())
        {
            ShaderEditorData& data = m_Shaders[m_Selected];
            if (data.previouslyOpened)
            {
                ImGui::PushFont(Pandora::GetImGuiSystem()->GetFont(Font::UBUNTU_MONO_18));
                data.editor.Render("Shader editor", ImVec2(), true);
                ImGui::PopFont();
            }
        }
    }
    ImGui::End();
}

void ShaderEditor::Show(bool state)
{
    m_Show = state;
}

void ShaderEditor::Initialize()
{
    m_Initialized = true;

    const static TextEditor::Palette palette = { {
			IM_COL32(230, 230, 230, 255), // None
			IM_COL32(  5, 250, 191, 255), // Keyword	
			IM_COL32(255, 140,   0, 255), // Number
			IM_COL32(230, 230, 230, 255), // String
			IM_COL32(230, 230, 230, 255), // Char literal
			IM_COL32(  5, 250, 191, 255), // Punctuation
			IM_COL32(230, 230, 230, 255), // Preprocessor
			IM_COL32(230, 230, 230, 255), // Identifier
			IM_COL32(230, 230, 230, 255), // Known identifier
			IM_COL32(230, 230, 230, 255), // Preproc identifier
			IM_COL32(160, 160, 160, 255), // Comment (single line)
			IM_COL32(160, 160, 160, 255), // Comment (multi line)
			IM_COL32( 46,  46,  46, 240), // Background
            IM_COL32(  5, 250, 191, 255), // Cursor
			IM_COL32(  5, 250, 191,  60), // Selection
			IM_COL32(160,   0,  16, 255), // ErrorMarker
			IM_COL32(160,   0,  16, 255), // Breakpoint
			IM_COL32(140, 140, 140, 255), // Line number
			IM_COL32(255, 255, 255,  20), // Current line fill
			IM_COL32(255, 255, 255,  20), // Current line fill (inactive)
			IM_COL32(  0,   0,   0,   0), // Current line edge
		} };

    auto shadersList = GetVFS()->List("/shaders");
    m_ShadersToLoad = shadersList.size();
    for (auto& shaderFile : shadersList)
    {
        GetResourceSystem()->RequestResource(shaderFile, [this](ResourceSharedPtr pResource) {
            ResourceShaderSharedPtr pResourceShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
            ShaderEditorData data{
                .pResource = pResourceShader,
                .state = ShaderState::Compiled,
                .previouslyOpened = false
            };

            data.editor.SetPalette(palette);
            data.editor.SetText(pResourceShader->GetShaderCode());
            data.code = data.editor.GetText();

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
    ImGui::BeginChild("ShaderList", ImVec2(shaderListWidth, ImGui::GetContentRegionAvail().y - compileButtonHeight - ImGui::GetStyle().ItemSpacing.y), ImGuiChildFlags_Border);

    for (auto& entry : m_Shaders)
    {
        const std::string& editorText = entry.second.editor.GetText();
        const std::string& lastSavedText = entry.second.code;
        const bool modified = (editorText != lastSavedText);
        const bool error = (entry.second.state == ShaderEditor::ShaderState::Error);

        std::string label;
        if (error)
        {
            label = ICON_FA_TRIANGLE_EXCLAMATION " " + entry.first;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0f, 1.0f));
        }
        else if (modified)
        {
            label = ICON_FA_FLOPPY_DISK " " + entry.first;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.5, 0.0f, 1.0f));
        }
        else
        {
            label = ICON_FA_FILE_CODE " " + entry.first;
        }

        bool selected = (entry.first == m_Selected);
        if (ImGui::Selectable(label.c_str(), &selected))
        {
            if (m_Selected != entry.first)
            {
                OpenShader(entry.first);
            }
        }

        if (error || modified)
        {
            ImGui::PopStyleColor();
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

    ShaderEditorData& data = m_Shaders[m_Selected];
    if (!data.previouslyOpened)
    {
        data.editor.SetText(data.code);
        data.editor.SetCursorPosition(TextEditor::Coordinates(0, 0));
        data.previouslyOpened = true;
    }
}

void ShaderEditor::CompileSelectedShader()
{
    if (m_Selected.empty())
    {
        return;
    }

    ShaderEditorData& data = m_Shaders[m_Selected];

    const std::string& codeToCompile = data.editor.GetText();
    data.pResource->Inject(codeToCompile, [this, &data, codeToCompile](ShaderCompilationResult* pResult){
        TextEditor::ErrorMarkers errorMarkers;
        if (pResult->GetState() == ShaderCompilationResult::State::Error)
        {
            data.state = ShaderState::Error;
            for (auto& error : pResult->GetErrors())
            {
                errorMarkers[static_cast<int>(error.GetLineNumber())] = error.GetMessage();
            }
        }
        else
        {
            data.code = codeToCompile;
            data.state = ShaderState::Compiled;
        }
        data.editor.SetErrorMarkers(errorMarkers);
    });
}

} // namespace WingsOfSteel::Pandora