#pragma once

#include "sub_sector/signal/sub_sector_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(AsteroidFieldSignal);
class AsteroidFieldSignal : public SubSectorSignal
{
public:
    AsteroidFieldSignal();
    ~AsteroidFieldSignal() override;

    SubSectorSharedPtr Spawn() const override;
};

} // namespace WingsOfSteel::TheBrightestStar