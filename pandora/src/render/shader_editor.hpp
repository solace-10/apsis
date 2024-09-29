#pragma once

#include <map>
#include <string>

#include "core/smart_ptr.hpp"

#include "imgui/text_editor/text_editor.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(ResourceShader);

class ShaderEditor
{
public:
    ShaderEditor();
    ~ShaderEditor();

    void Update();
    void Show(bool state);

private:
    void Initialize();
    void DrawShaderList();
    void OpenShader(const std::string& shader);
    void CompileSelectedShader();

    TextEditor m_TextEditor;

    enum class ShaderState
    {
        Compiled,
        Modified,
        Error
    };

    struct ShaderEditorData
    {
        ResourceShaderSharedPtr pResource;
        std::string code;
        ShaderState state;
    };

    std::map<std::string, ShaderEditorData> m_Shaders;
    bool m_Initialized{ false };
    bool m_Show{ false };
    std::string m_Selected;
    int m_ShadersToLoad{ 0 };
};

} // namespace WingsOfSteel::Pandora