#pragma once

#include <vector>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SubSectorSignal);

DECLARE_SMART_PTR(Sector);
class Sector
{
public:
    Sector() {}
    virtual ~Sector() {}

    virtual void Initialize() = 0;
    virtual std::vector<SubSectorSignal*> GetSignals() const = 0;
};

} // namespace WingsOfSteel::TheBrightestStar