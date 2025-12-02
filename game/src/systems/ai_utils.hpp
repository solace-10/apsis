#pragma once

#include <optional>
#include <string>

#include <scene/entity.hpp>

#include "components/faction_component.hpp"
#include "components/threat_component.hpp"

namespace WingsOfSteel
{

class AIUtils
{
public:
    enum class TargetRangeOrder
    {
        Closest,
        ClosestToCarrier
    };
  
    static EntitySharedPtr AcquireTarget(EntitySharedPtr pAcquiringEntity, const std::vector<ThreatCategory>& targetCategoryOrder, TargetRangeOrder targetRangeOrder);
    static float CalculateOptimalRange(EntitySharedPtr pMechEntity);
    static Faction GetOppositeFaction(Faction faction);
    static EntitySharedPtr GetCarrier(Faction faction);

private:
    static std::optional<size_t> GetThreatIndex(const std::vector<ThreatCategory>& targetCategoryOrder, ThreatCategory targetCategory);
};
  
} // namespace WingsOfSteel
