#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include <core/smart_ptr.hpp>

#include "ui/element.hpp"
#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Window);
class Window : public Element
{
public:
    Window();
    ~Window() override {}

    virtual void Initialize();
    void LoadPrefab(const std::string& prefabPath);
    void Render() override;

    void AddElement(ElementSharedPtr pElement);

    void Bind(const std::string& key, std::function<void(const PrefabDataValue&)> callback);

    void SetTitle(const std::string& title);

private:
    void RenderBackground();

    PrefabDataSharedPtr m_pPrefabData;
    std::unordered_map<std::string, std::function<void(const PrefabDataValue&)>> m_Bindings;

    std::string m_Title{"Window"};
    std::vector<ElementSharedPtr> m_Elements;
};

} // namespace WingsOfSteel::TheBrightestStar::UI