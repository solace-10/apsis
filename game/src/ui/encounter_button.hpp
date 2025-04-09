#pragma once

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "ui/events.hpp"
#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(EncounterButton);
class EncounterButton : public StackableElement
{
public:
    EncounterButton();
    ~EncounterButton() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetText(const std::string& text);
    const std::string& GetText() const;

    void SetOnClickedEvent(const OnClickedEvent& event);

private:
    void SetIconSource(const std::string& source);

    std::string m_Text;
    int m_Margin;
    Pandora::ResourceTexture2DSharedPtr m_pIconTexture;
    Property<std::string> m_IconSource;
    OnClickedEvent m_OnClickedEvent;
};

inline void EncounterButton::SetOnClickedEvent(const OnClickedEvent& event)
{
    m_OnClickedEvent = event;
}

} // namespace WingsOfSteel::TheBrightestStar::UI