#pragma once

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SubSector);

DECLARE_SMART_PTR(SubSectorSignal);
class SubSectorSignal
{
public:
    SubSectorSignal() {}
    virtual ~SubSectorSignal() {}

    virtual SubSectorSharedPtr Spawn() const = 0;
};

} // namespace WingsOfSteel::TheBrightestStar