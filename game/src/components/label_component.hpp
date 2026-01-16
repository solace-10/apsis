#pragma once

#include <string>

#include <glm/vec2.hpp>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class LabelComponent : public IComponent
{
public:
    LabelComponent() = default;
    
    LabelComponent(const std::string& text)
    : m_Text(text)
    {}
    
    ~LabelComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    void SetText(const std::string& text) { m_Text = text; }
    const std::string& GetText() const { return m_Text; }
    void SetScreenSpacePosition(const glm::vec2& position) { m_ScreenSpacePosition = position; }
    const glm::vec2& GetScreenSpacePosition() const { return m_ScreenSpacePosition; }

private:
    std::string m_Text{ "UNKNOWN" };
    glm::vec2 m_ScreenSpacePosition{ 0.0f };
};

REGISTER_COMPONENT(LabelComponent, "label")

} // namespace WingsOfSteel
