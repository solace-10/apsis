#include "sector/signal/sector_signal.hpp"

#include "sector/custom_sector.hpp"

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
    SectorSharedPtr pSector = std::make_shared<CustomSector>();
    pSector->Initialize();
    return pSector;
}

} // namespace WingsOfSteel::TheBrightestStar