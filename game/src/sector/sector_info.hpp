#pragma once

#include <string>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

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
    SectorInfo(int level);
    ~SectorInfo();

    uint32_t GetSeed() const;

private:
    uint32_t m_Seed;
};

} // namespace WingsOfSteel::TheBrightestStar