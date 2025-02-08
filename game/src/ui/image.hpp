#pragma once

#include <core/smart_ptr.hpp>

#include "ui/element.hpp"
#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Image);
class Image : public Element
{
public:
    Image() {}
    ~Image() override {}

    void Render() override;

private:

};

} // namespace WingsOfSteel::TheBrightestStar::UI