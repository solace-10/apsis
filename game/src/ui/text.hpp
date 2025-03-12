#pragma once

#include <core/smart_ptr.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Text);
class Text : public StackableElement
{
public:
    Text();
    ~Text() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetText(const std::string& text);
    const std::string& GetText() const;

private:
    std::string m_Text;
    int m_Margin;
};

} // namespace WingsOfSteel::TheBrightestStar::UI