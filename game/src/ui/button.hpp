#pragma once

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "ui/events.hpp"
#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Button);
class Button : public StackableElement
{
public:
    Button();
    ~Button() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetText(const std::string& text);
    const std::string& GetText() const;

    void SetOnClickedEvent(const OnClickedEvent& event);

    enum class Mode
    {
        Standard,
        Important
    };
    
private:
    void SetIconSource(const std::string& source);

    Mode m_Mode{Mode::Standard};
    std::string m_Text;
    int m_Margin{0};
    Pandora::ResourceTexture2DSharedPtr m_pIconTexture;
    Property<std::string> m_IconSource;
    OnClickedEvent m_OnClickedEvent;
    float m_BackgroundAnimation{0.0f};
    float m_BackgroundOffset{0.0f};
};

inline void Button::SetOnClickedEvent(const OnClickedEvent& event)
{
    m_OnClickedEvent = event;
}

} // namespace WingsOfSteel::TheBrightestStar::UI