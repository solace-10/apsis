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

        if (!m_Selected.empty())
        {
            ShaderEditorData& data = m_Shaders[m_Selected];
            if (data.previouslyOpened)
            {
                data.editor.Render("Shader editor", ImVec2(), true);
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
			0xff7f7f7f,	// None
			0xffff0c06,	// Keyword	
			0xff008000,	// Number
			0xff2020a0,	// String
			0xff304070, // Char literal
			0xff000000, // Punctuation
			0xff406060,	// Preprocessor
			0xff404040, // Identifier
			0xff606010, // Known identifier
			0xffc040a0, // Preproc identifier
			0xff205020, // Comment (single line)
			0xff405020, // Comment (multi line)
			IM_COL32(255, 250, 242, 148), // Background
            IM_COL32(  0,   0,   0, 255), // Cursor
			0x80600000, // Selection
			0xa00010ff, // ErrorMarker
			0x80f08000, // Breakpoint
			0xff505000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40000000, // Current line edge
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