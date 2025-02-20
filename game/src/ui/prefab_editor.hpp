#pragma once

#include <optional>
#include <vector>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Element);
DECLARE_SMART_PTR(Window);

class PrefabEditor
{
public:
    PrefabEditor();
    ~PrefabEditor();

    void Initialize();
    void ShowPrefabEditor(bool state);
    void DrawPrefabEditor();

    void AddPrefabData(const std::string& prefabPath, WindowSharedPtr pWindow);

private:
    void RenderHierarchy();
    void RenderTreeElement(ElementSharedPtr pElement);
    void RenderProperties();
    void Save();
    void Revert();

    bool m_ShowPrefabEditor{false};

    struct RegisteredPrefab
    {
        std::string prefabPath;
        WindowWeakPtr pWindow;
    };

    std::vector<RegisteredPrefab> m_RegisteredPrefabs;
    std::optional<RegisteredPrefab> m_pSelectedPrefab;
    ElementWeakPtr m_pSelectedElement;
};

} // namespace WingsOfSteel::TheBrightestStar::UI