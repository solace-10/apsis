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

    void SetDynamic(bool isDynamic);
    bool IsDynamic() const;

private:
    std::string m_Text{"<placeholder>"};
    bool m_IsDynamic{false};
    bool m_Underlined{true};
};

inline void Heading::SetText(const std::string& text)
{
    m_Text = text;
}

inline const std::string& Heading::GetText() const
{
    return m_Text;
}

inline void Heading::SetDynamic(bool isDynamic)
{
    m_IsDynamic = isDynamic;
}

inline bool Heading::IsDynamic() const
{
    return m_IsDynamic;
}

} // namespace WingsOfSteel::TheBrightestStar::UI