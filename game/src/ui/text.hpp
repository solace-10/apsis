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

    void SetScrollable(bool isScrollable);
    bool IsScrollable() const;

    void SetMarkdown(bool isMarkdown);
    bool IsMarkdown() const;

    void SetDynamic(bool isDynamic);
    bool IsDynamic() const;

    //static const 

private:
    std::string m_Text;
    int m_Margin{0};
    bool m_IsScrollable{false};
    bool m_IsMarkdown{false};
    bool m_IsDynamic{false};
};

inline const std::string& Text::GetText() const
{
    return m_Text;
}

inline void Text::SetScrollable(bool isScrollable)
{
    m_IsScrollable = isScrollable;
}

inline bool Text::IsScrollable() const
{
    return m_IsScrollable;
}

inline void Text::SetMarkdown(bool isMarkdown)
{
    m_IsMarkdown = isMarkdown;
}

inline bool Text::IsMarkdown() const
{
    return m_IsMarkdown;
}

inline void Text::SetDynamic(bool isDynamic)
{
    m_IsDynamic = isDynamic;
}

inline bool Text::IsDynamic() const
{
    return m_IsDynamic;
}

} // namespace WingsOfSteel::TheBrightestStar::UI