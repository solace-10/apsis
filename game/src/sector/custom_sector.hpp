#pragma once

#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SubSectorSignal);

DECLARE_SMART_PTR(CustomSector);
class CustomSector : public Sector
{
public:
    CustomSector();
    ~CustomSector() override;

    void Initialize() override;
    std::vector<SubSectorSignal*> GetSignals() const override;

private:
    SubSectorSignalUniquePtr m_pSignal;
};

} // namespace WingsOfSteel::TheBrightestStar