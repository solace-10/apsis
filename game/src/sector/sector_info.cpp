// #include <glm/glm.hpp>

// #include "sector/signal/signal.hpp"
// #include "sector/sector_info.hpp"
// #include "sector/sub_sector_info.hpp"

// namespace WingsOfSteel::TheBrightestStar
// {

// SectorInfo::SectorInfo(int level, const glm::vec2& size, const std::vector<SignalSharedPtr>& signals)
// : m_Level(level)
// , m_Size(size)
// , m_Signals(signals)
// {

// }

// const std::vector<SignalSharedPtr>& SectorInfo::GetSignals() const
// {
//     return m_Signals;
// }

// SubSectorInfoSharedPtr SectorInfo::CreateSubSectorInfo()
// {
//     glm::vec2 coordinates(0.0f);
//     std::vector<SignalSharedPtr> signalsToSpawn;
//     for (auto& pSignal : m_Signals)
//     {
//         const std::string& name = pSignal->GetName();
//         if (name == "Asteroid field" || name == "Rich asteroid field")
//         {
//             coordinates = glm::trunc(pSignal->GetPosition());
//             signalsToSpawn.push_back(pSignal);
//             break;
//         }
//     }

//     return std::make_shared<SubSectorInfo>(shared_from_this(), coordinates, signalsToSpawn);
// }

// } // namespace WingsOfSteel::TheBrightestStar