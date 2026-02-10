#if defined(TARGET_PLATFORM_WEB)

#include <cmath>

#include <emscripten/val.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <core/log.hpp>
#include <scene/entity.hpp>

#include "components/metadata_component.hpp"
#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "emscripten/web_interop.hpp"
#include "game.hpp"
#include "sector/group_filters.hpp"
#include "sector/sector.hpp"
#include "systems/space_object_render_system.hpp"

namespace WingsOfSteel
{

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

void WebInterop::NotifySpaceObjectSelected(EntitySharedPtr pEntity)
{
    if (!pEntity)
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

    const MetadataComponent& metadata = pEntity->GetComponent<MetadataComponent>();
    const OrbitalElementsComponent& orbitalElements = pEntity->GetComponent<OrbitalElementsComponent>();
    const OrbitalStateComponent& orbitalState = pEntity->GetComponent<OrbitalStateComponent>();

    emscripten::val interop = emscripten::val::object();
    interop.set("objectName", metadata.m_ObjectName);
    interop.set("objectId", metadata.m_ObjectId);
    interop.set("noradCatalogueId", metadata.m_NoradCatalogueId);
    interop.set("eccentricity", orbitalElements.GetEccentricity());
    interop.set("inclination", orbitalElements.GetInclination());
    interop.set("rightAscensionOfAscendingNode", orbitalElements.GetRightAscensionOfAscendingNode());
    interop.set("argumentOfPericenter", orbitalElements.GetArgumentOfPericenter());
    interop.set("meanAnomaly", orbitalElements.GetMeanAnomaly());
    interop.set("semiMajorAxis", static_cast<float>(orbitalState.m_SemiMajorAxis));
    interop.set("altitude", static_cast<float>(orbitalState.m_Altitude));
    interop.set("velocity", static_cast<float>(orbitalState.m_Velocity));
    interop.set("latitude", static_cast<float>(orbitalState.m_Latitude));
    interop.set("longitude", static_cast<float>(orbitalState.m_Longitude));

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

void WebInterop::NotifySpaceObjectUpdated(EntitySharedPtr pEntity)
{
    if (!pEntity)
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

    const MetadataComponent& metadata = pEntity->GetComponent<MetadataComponent>();
    const OrbitalElementsComponent& orbitalElements = pEntity->GetComponent<OrbitalElementsComponent>();
    const OrbitalStateComponent& orbitalState = pEntity->GetComponent<OrbitalStateComponent>();

    emscripten::val interop = emscripten::val::object();
    interop.set("objectName", metadata.m_ObjectName);
    interop.set("objectId", metadata.m_ObjectId);
    interop.set("noradCatalogueId", metadata.m_NoradCatalogueId);
    interop.set("eccentricity", orbitalElements.GetEccentricity());
    interop.set("inclination", orbitalElements.GetInclination());
    interop.set("rightAscensionOfAscendingNode", orbitalElements.GetRightAscensionOfAscendingNode());
    interop.set("argumentOfPericenter", orbitalElements.GetArgumentOfPericenter());
    interop.set("meanAnomaly", orbitalElements.GetMeanAnomaly());
    interop.set("semiMajorAxis", static_cast<float>(orbitalState.m_SemiMajorAxis));
    interop.set("altitude", static_cast<float>(orbitalState.m_Altitude));
    interop.set("velocity", static_cast<float>(orbitalState.m_Velocity));
    interop.set("latitude", static_cast<float>(orbitalState.m_Latitude));
    interop.set("longitude", static_cast<float>(orbitalState.m_Longitude));

    onUpdated(interop);
}

void WebInterop::NotifyGroupFiltersChanged(GroupFilters* pGroupFilters)
{
    if (!pGroupFilters)
    {
        return;
    }

    emscripten::val callbacks = emscripten::val::global("orbisCallbacks");
    if (callbacks.isUndefined() || callbacks.isNull())
    {
        return;
    }

    emscripten::val onChanged = callbacks["onGroupFiltersChanged"];
    if (onChanged.isUndefined())
    {
        return;
    }

    emscripten::val groupsArray = emscripten::val::array();
    for (const std::string& name : pGroupFilters->GetGroupFilterNames())
    {
        GroupFilter* pFilter = pGroupFilters->GetGroupFilter(name);
        if (!pFilter)
        {
            continue;
        }

        emscripten::val group = emscripten::val::object();
        group.set("id", pFilter->GetName());
        group.set("name", pFilter->GetDisplayName());
        group.set("color", pFilter->GetHexColor());
        group.set("visible", pFilter->IsEnabled());
        group.set("count", pFilter->GetCount());
        groupsArray.call<void>("push", group);
    }

    onChanged(groupsArray);
}

void WebInterop::SetGroupFilterEnabled(const std::string& groupId, bool enabled)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    GroupFilters* pGroupFilters = pSector->GetGroupFilters();
    if (!pGroupFilters)
    {
        return;
    }

    GroupFilter* pFilter = pGroupFilters->GetGroupFilter(groupId);
    if (!pFilter)
    {
        Log::Warning() << "SetGroupFilterEnabled: unknown group '" << groupId << "'.";
        return;
    }

    pFilter->SetEnabled(enabled);

    if (WebInterop* pWebInterop = GetInstance())
    {
        pWebInterop->NotifyGroupFiltersChanged(pGroupFilters);
    }

    SpaceObjectRenderSystem* pSpaceObjectRenderSystem = pSector->GetSystem<SpaceObjectRenderSystem>();
    if (pSpaceObjectRenderSystem)
    {
        pSpaceObjectRenderSystem->NotifyGroupFiltersChanged();
    }
}

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
