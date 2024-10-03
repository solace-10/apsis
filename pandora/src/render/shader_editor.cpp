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
			0x20ffffff, // Background
			0xff000000, // Cursor
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
                .code = pResourceShader->GetShaderCode(),
                .originalCode = pResourceShader->GetShaderCode(),
                .state = ShaderState::Compiled,
                .previouslyOpened = false
            };

            data.editor.SetPalette(palette);

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
    data.code = data.editor.GetText();

    // TODO: Inject and send to compiler.
    // TODO: Add a callback for error handling

    data.pResource->Inject(data.code, [this, &data](ShaderCompilationResult* pResult){
        int a = 0;
    });
}

} // namespace WingsOfSteel::Pandora