#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorInfo);
DECLARE_SMART_PTR(Signal);

DECLARE_SMART_PTR(SubSectorInfo);
class SubSectorInfo
{
public:
    SubSectorInfo(SectorInfoWeakPtr pSectorInfo, const glm::vec2& coordinates, const std::vector<SignalSharedPtr>& signals)
    : m_pSectorInfo(pSectorInfo)
    , m_Coordinates(coordinates)
    , m_Signals(signals)
    {}

    ~SubSectorInfo() {}

    SectorInfoWeakPtr GetSectorInfo() const;
    const glm::vec2& GetCoordinates() const;
    const std::vector<SignalSharedPtr>& GetSignals() const;

private:
    SectorInfoWeakPtr m_pSectorInfo;
    glm::vec2 m_Coordinates;
    std::vector<SignalSharedPtr> m_Signals;
};

inline SectorInfoWeakPtr SubSectorInfo::GetSectorInfo() const
{
    return m_pSectorInfo;
}

inline const glm::vec2& SubSectorInfo::GetCoordinates() const
{
    return m_Coordinates;
}

inline const std::vector<SignalSharedPtr>& SubSectorInfo::GetSignals() const
{
    return m_Signals;
}

} // namespace WingsOfSteel::TheBrightestStar