#include "sector/custom_sector.hpp"

#include "sub_sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

CustomSector::CustomSector()
{

}
    
CustomSector::~CustomSector()
{

}

void CustomSector::Initialize()
{
    m_pSignal = std::make_unique<AsteroidFieldSignal>();
}

std::vector<SubSectorSignal*> CustomSector::GetSignals() const
{
    std::vector<SubSectorSignal*> signals;
    signals.push_back(m_pSignal.get());
    return signals;
}

} // namespace WingsOfSteel::TheBrightestStar