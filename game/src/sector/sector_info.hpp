// #pragma once

// #include <string>
// #include <vector>

// #include <glm/vec2.hpp>

// #include <core/smart_ptr.hpp>

// namespace WingsOfSteel::TheBrightestStar
// {

// DECLARE_SMART_PTR(Signal);

// // clang-format off
// enum class SectorCivilizationLevel
// {
//     Uncolonized, // No life has ever been present in this sector.
//     Decivilized, // Something catastrophic has happened here: it used to be colonized, but all that remains are ruins.
//     Outposts,    // Minimal presence, perhaps a research or military outpost but no large colonies.
//     Populated,   // One or more worlds are well populated.
//     Colonized,   // The entire system has been colonized.
// };
// // clang-format on

// DECLARE_SMART_PTR(SubSectorInfo);

// DECLARE_SMART_PTR(SectorInfo);
// class SectorInfo : public std::enable_shared_from_this<SectorInfo>
// {
// public:
//     SectorInfo(int level, const glm::vec2& size, const std::vector<SignalSharedPtr>& signals);
//     ~SectorInfo() {}

//     const std::vector<SignalSharedPtr>& GetSignals() const;

//     // This should receive the coordinates the player is jumping to, but for prototyping reasons
//     // we make sure we always return a sector with an asteroid field in it.
//     SubSectorInfoSharedPtr CreateSubSectorInfo();

// private:
//     int m_Level{ 0 };
//     glm::vec2 m_Size{ 1.0f };
//     std::vector<SignalSharedPtr> m_Signals;
// };

// } // namespace WingsOfSteel::TheBrightestStar