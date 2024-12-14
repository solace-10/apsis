#include "hyperscape/hyperscape.hpp"

#include "sector/signal/sector_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{


Hyperscape::Hyperscape()
{

}

Hyperscape::~Hyperscape()
{

}

void Hyperscape::Initialize()
{
    m_pSignal = std::make_unique<SectorSignal>();
}

std::vector<SectorSignal*> Hyperscape::GetSignals() const
{
    std::vector<SectorSignal*> signals;
    signals.push_back(m_pSignal.get());
    return signals;
}

} // namespace WingsOfSteel::TheBrightestStar
