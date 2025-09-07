#pragma once

#include <core/smart_ptr.hpp>

#include "ui/stackable_element.hpp"

namespace WingsOfSteel::UI
{

DECLARE_SMART_PTR(Panel);
class Panel : public StackableElement
{
public:
    Panel();
    ~Panel() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
};

} // namespace WingsOfSteel::UI