#pragma once

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Sector);

DECLARE_SMART_PTR(SectorSignal);
class SectorSignal
{
public:
    SectorSignal();
    virtual ~SectorSignal();

    SectorSharedPtr Spawn() const;
};

} // namespace WingsOfSteel::TheBrightestStar