#if defined(TARGET_PLATFORM_WEB)

#include <cmath>

#include <emscripten/val.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <core/log.hpp>

#include "emscripten/web_interop.hpp"
#include "space_objects/space_object.hpp"
#include "systems/orbit_simulation_system.hpp"

namespace WingsOfSteel
{

// Earth's gravitational parameter (km³/s²)
static constexpr double kMu = 398600.4418;

// Earth's mean radius (km)
static constexpr double kEarthMeanRadius = 6371.0;

WebInterop* WebInterop::s_pInstance = nullptr;

WebInterop::WebInterop()
{
    s_pInstance = this;
    Log::Info() << "WebInterop initialized.";
}

WebInterop::~WebInterop()
{
    s_pInstance = nullptr;
}

WebInterop* WebInterop::GetInstance()
{
    return s_pInstance;
}

void WebInterop::NotifySpaceObjectSelected(const SpaceObject* pSpaceObject)
{
    if (pSpaceObject == nullptr)
    {
        NotifySpaceObjectDeselected();
        return;
    }

    emscripten::val callbacks = emscripten::val::global("orbisCallbacks");
    if (callbacks.isUndefined() || callbacks.isNull())
    {
        return;
    }

    emscripten::val onSelected = callbacks["onSpaceObjectSelected"];
    if (onSelected.isUndefined())
    {
        return;
    }

    // Calculate current position and derived values
    const glm::dvec3 position = OrbitSimulationSystem::CalculateCartesianPosition(*pSpaceObject);
    const double altitude = glm::length(position) - kEarthMeanRadius;

    // Calculate velocity from vis-viva equation: v² = μ(2/r - 1/a)
    const double n = pSpaceObject->GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;
    const double semiMajorAxis = std::cbrt(kMu / (n * n));
    const double r = glm::length(position);
    const double velocity = std::sqrt(kMu * (2.0 / r - 1.0 / semiMajorAxis));

    // Get lat/lon
    const glm::dvec2 latLon = OrbitSimulationSystem::ECIToLatLon(position);

    emscripten::val interop = emscripten::val::object();
    interop.set("objectName", pSpaceObject->GetObjectName());
    interop.set("objectId", pSpaceObject->GetObjectId());
    interop.set("noradCatalogueId", pSpaceObject->GetNoradCatalogueId());
    interop.set("eccentricity", pSpaceObject->GetEccentricity());
    interop.set("inclination", pSpaceObject->GetInclination());
    interop.set("rightAscensionOfAscendingNode", pSpaceObject->GetRightAscensionOfAscendingNode());
    interop.set("argumentOfPericenter", pSpaceObject->GetArgumentOfPericenter());
    interop.set("meanAnomaly", pSpaceObject->GetMeanAnomaly());
    interop.set("semiMajorAxis", static_cast<float>(semiMajorAxis));
    interop.set("altitude", static_cast<float>(altitude));
    interop.set("velocity", static_cast<float>(velocity));
    interop.set("latitude", static_cast<float>(latLon.x));
    interop.set("longitude", static_cast<float>(latLon.y));

    onSelected(interop);
}

void WebInterop::NotifySpaceObjectDeselected()
{
    emscripten::val callbacks = emscripten::val::global("orbisCallbacks");
    if (callbacks.isUndefined() || callbacks.isNull())
    {
        return;
    }

    emscripten::val onDeselected = callbacks["onSpaceObjectDeselected"];
    if (onDeselected.isUndefined())
    {
        return;
    }

    onDeselected();
}

void WebInterop::NotifySpaceObjectUpdated(const SpaceObject* pSpaceObject)
{
    if (pSpaceObject == nullptr)
    {
        return;
    }

    emscripten::val callbacks = emscripten::val::global("orbisCallbacks");
    if (callbacks.isUndefined() || callbacks.isNull())
    {
        return;
    }

    emscripten::val onUpdated = callbacks["onSpaceObjectUpdated"];
    if (onUpdated.isUndefined())
    {
        return;
    }

    // Calculate current position and derived values
    const glm::dvec3 position = OrbitSimulationSystem::CalculateCartesianPosition(*pSpaceObject);
    const double altitude = glm::length(position) - kEarthMeanRadius;

    // Calculate velocity from vis-viva equation
    const double n = pSpaceObject->GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;
    const double semiMajorAxis = std::cbrt(kMu / (n * n));
    const double r = glm::length(position);
    const double velocity = std::sqrt(kMu * (2.0 / r - 1.0 / semiMajorAxis));

    // Get lat/lon
    const glm::dvec2 latLon = OrbitSimulationSystem::ECIToLatLon(position);

    emscripten::val interop = emscripten::val::object();
    interop.set("objectName", pSpaceObject->GetObjectName());
    interop.set("objectId", pSpaceObject->GetObjectId());
    interop.set("noradCatalogueId", pSpaceObject->GetNoradCatalogueId());
    interop.set("eccentricity", pSpaceObject->GetEccentricity());
    interop.set("inclination", pSpaceObject->GetInclination());
    interop.set("rightAscensionOfAscendingNode", pSpaceObject->GetRightAscensionOfAscendingNode());
    interop.set("argumentOfPericenter", pSpaceObject->GetArgumentOfPericenter());
    interop.set("meanAnomaly", pSpaceObject->GetMeanAnomaly());
    interop.set("semiMajorAxis", static_cast<float>(semiMajorAxis));
    interop.set("altitude", static_cast<float>(altitude));
    interop.set("velocity", static_cast<float>(velocity));
    interop.set("latitude", static_cast<float>(latLon.x));
    interop.set("longitude", static_cast<float>(latLon.y));

    onUpdated(interop);
}

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
