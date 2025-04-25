#pragma once

#include <core/smart_ptr.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Heading);
class Heading : public StackableElement
{
public:
    Heading();
    ~Heading() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetText(const std::string& text);
    const std::string& GetText() const;

    enum class HeadingLevel
    {
        Heading1,
        Heading2
    };

private:
    std::string m_Text{"<placeholder>"};
    HeadingLevel m_HeadingLevel{HeadingLevel::Heading1};
};

inline void Heading::SetText(const std::string& text)
{
    m_Text = text;
}

inline const std::string& Heading::GetText() const
{
    return m_Text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI