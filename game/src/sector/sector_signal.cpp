#include "sector/sector_signal.hpp"

#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SectorSignal::SectorSignal()
{

}
    
SectorSignal::~SectorSignal()
{

}

SectorSharedPtr SectorSignal::Spawn() const
{
    SectorSharedPtr pSector = std::make_shared<Sector>();
    pSector->Initialize();
    return pSector;
}

} // namespace WingsOfSteel::TheBrightestStar