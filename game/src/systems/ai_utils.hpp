#pragma once

#include <optional>
#include <string>

#include <scene/entity.hpp>

namespace WingsOfSteel
{

class AIUtils
{
public:
    enum class TargetPriorityOrder
    {
        Closest,
        ClosestToCarrier
    };
  
    static EntitySharedPtr AcquireTarget(EntitySharedPtr pAcquiringEntity, TargetPriorityOrder targetPriorityOrder, std::optional<std::string> priorityThreatType = std::nullopt);
    static float CalculateOptimalRange(EntitySharedPtr pMechEntity);
};
  
} // namespace WingsOfSteel
