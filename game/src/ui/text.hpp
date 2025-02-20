#pragma once

#include <core/smart_ptr.hpp>

#include "ui/element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Text);
class Text : public Element
{
public:
    Text() {}
    ~Text() override {}

    void Render() override;

    void SetText(const std::string& text);
    const std::string& GetText() const;

private:
    std::string m_Text;
};

} // namespace WingsOfSteel::TheBrightestStar::UI