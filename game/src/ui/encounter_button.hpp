#pragma once

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

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

private:
    void SetIconSource(const std::string& source);

    std::string m_Text;
    int m_Margin;
    Pandora::ResourceTexture2DSharedPtr m_pIconTexture;
    Property<std::string> m_IconSource;
};

} // namespace WingsOfSteel::TheBrightestStar::UI