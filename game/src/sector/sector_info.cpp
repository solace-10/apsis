#include "sector/sector_info.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SectorInfo::SectorInfo(int level, const std::vector<SignalSharedPtr>& signals)
: m_Level(level)
, m_Signals(signals)
{

}

SectorInfo::~SectorInfo()
{

}


const std::vector<SignalSharedPtr>& SectorInfo::GetSignals() const
{
    return m_Signals;
}

} // namespace WingsOfSteel::TheBrightestStar