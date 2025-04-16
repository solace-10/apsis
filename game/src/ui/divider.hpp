#pragma once

#include <core/smart_ptr.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Divider);
class Divider : public StackableElement
{
public:
    Divider();
    ~Divider() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
};

} // namespace WingsOfSteel::TheBrightestStar::UI