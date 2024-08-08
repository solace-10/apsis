#pragma once

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorInfo);

DECLARE_SMART_PTR(Sector);
class Sector
{
public:
    Sector(SectorInfoSharedPtr& pSectorInfo);
    ~SectorInfo();

    void Update(float delta);

private:
    SectorSharedPtr m_pSectorInfo;
};

} // namespace WingsOfSteel::TheBrightestStar