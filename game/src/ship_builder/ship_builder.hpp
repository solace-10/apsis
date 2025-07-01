#pragma once

#include <string>

#include <glm/mat4x4.hpp>

#include <core/result.hpp>
#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>

#include "components/hardpoint_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(ShipBuilder);
class ShipBuilder
{
public:
    static void Build(Pandora::EntitySharedPtr pShip, const glm::mat4& worldTransform);

    enum EAddWeaponResult
    {
        Ok,
        InvalidSector,
        InvalidHardpoint
    };

private:
    static bool AddHardpoint(Pandora::EntitySharedPtr pShip, const std::string& attachmentPointName, float minArc, float maxArc);
    static Pandora::Result<EAddWeaponResult> AddWeapon(Pandora::EntitySharedPtr pShip, const std::string& hardpointName, const std::string& weaponName);
    static bool FindHardpoint(Pandora::EntitySharedPtr pShip, const std::string& hardpointName, Hardpoint& outHardpoint);
};

} // namespace WingsOfSteel::TheBrightestStar