#if defined(TARGET_PLATFORM_WEB)

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <core/log.hpp>

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
    float semiMajorAxis{ 0.0f };
    float altitude{ 0.0f };
    float velocity{ 0.0f };
    float latitude{ 0.0f };
    float longitude{ 0.0f };
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
        .field("noradCatalogueId", &SpaceObjectInterop::noradCatalogueId)
        .field("semiMajorAxis", &SpaceObjectInterop::semiMajorAxis)
        .field("altitude", &SpaceObjectInterop::altitude)
        .field("velocity", &SpaceObjectInterop::velocity)
        .field("latitude", &SpaceObjectInterop::latitude)
        .field("longitude", &SpaceObjectInterop::longitude);
}

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
