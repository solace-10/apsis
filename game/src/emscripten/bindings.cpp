#if defined(TARGET_PLATFORM_WEB)

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <core/log.hpp>

#include "space_objects/space_object.hpp"

namespace WingsOfSteel
{

struct SpaceObjectInterop
{
    std::string objectName{ "UNKNOWN" };
    std::string objectId{ "0" };
    float eccentricity{ 0.0f };
    float inclination{ 0.0f };
    float rightAscensionOfAscendingNode{ 0.0f };
    float argumentOfPericenter{ 0.0f };
    float meanAnomaly{ 0.0f };
    uint32_t noradCatalogueId{ 0 };

    static SpaceObjectInterop Create(const SpaceObject& spaceObject)
    {
        return SpaceObjectInterop{
            .objectName = spaceObject.GetObjectName(),
            .objectId = spaceObject.GetObjectId(),
            .eccentricity = spaceObject.GetEccentricity(),
            .inclination = spaceObject.GetInclination(),
            .rightAscensionOfAscendingNode = spaceObject.GetRightAscensionOfAscendingNode(),
            .argumentOfPericenter = spaceObject.GetArgumentOfPericenter(),
            .meanAnomaly = spaceObject.GetMeanAnomaly(),
            .noradCatalogueId = spaceObject.GetNoradCatalogueId()
        };
    }
};

EMSCRIPTEN_BINDINGS(orbis)
{
    emscripten::value_object<SpaceObjectInterop>("SpaceObject")
        .field("objectName", &SpaceObjectInterop::objectName)
        .field("objectId", &SpaceObjectInterop::objectId)
        .field("eccentricity", &SpaceObjectInterop::eccentricity)
        .field("inclination", &SpaceObjectInterop::inclination)
        .field("rightAscensionOfAscendingNode", &SpaceObjectInterop::rightAscensionOfAscendingNode)
        .field("argumentOfPericenter", &SpaceObjectInterop::argumentOfPericenter)
        .field("meanAnomaly", &SpaceObjectInterop::meanAnomaly)
        .field("noradCatalogueId", &SpaceObjectInterop::noradCatalogueId);
}

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
