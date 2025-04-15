#pragma once

#include <glm/vec2.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "ui/events.hpp"
#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(EncounterDiceSelector);
class EncounterDiceSelector : public StackableElement
{
public:
    EncounterDiceSelector();
    ~EncounterDiceSelector() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

private:
    void RenderBackground(const glm::vec2& tl, const glm::vec2& br);

    Pandora::ResourceTexture2DSharedPtr m_pIconTexture;
    OnClickedEvent m_OnClickedEvent;
};

} // namespace WingsOfSteel::TheBrightestStar::UI