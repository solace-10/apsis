#pragma once

#include <core/smart_ptr.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::UI
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

    enum class Alignment
    {
        Left,
        Center,
        Right
    };

    enum class Mode
    {
        SingleLine,
        MultiLine,
        Markdown
    };

private:
    std::string m_Text;
    int m_Margin{0};
    Mode m_Mode{Mode::SingleLine};
    bool m_IsScrollable{false};
    Alignment m_Alignment{Alignment::Left};
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

} // namespace WingsOfSteel::UI