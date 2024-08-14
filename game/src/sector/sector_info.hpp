#pragma once

#include <string>
#include <vector>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Signal);

// clang-format off
enum class SectorCivilizationLevel
{
    Uncolonized, // No life has ever been present in this sector.
    Decivilized, // Something catastrophic has happened here: it used to be colonized, but all that remains are ruins.
    Outposts,    // Minimal presence, perhaps a research or military outpost but no large colonies.
    Populated,   // One or more worlds are well populated.
    Colonized,   // The entire system has been colonized.
};
// clang-format on

DECLARE_SMART_PTR(SectorInfo);
class SectorInfo
{
public:
    SectorInfo(int level, const std::vector<SignalSharedPtr>& signals);
    ~SectorInfo();

    const std::vector<SignalSharedPtr>& GetSignals() const;

private:
    int m_Level;
    std::vector<SignalSharedPtr> m_Signals;
};

} // namespace WingsOfSteel::TheBrightestStar