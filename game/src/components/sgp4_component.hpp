#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

#include "space/sgp4.hpp"

namespace WingsOfSteel
{

// The time-independent half of SGP4, derived once from the object's element set.
//
// It lives on the entity rather than in the system that uses it because it cannot change: the
// coefficients are a pure function of the mean elements, which are deserialized when the object is
// created and never touched again. Toggling a group filter rebuilds the roster of visible objects
// - see SpaceObjectRenderSystem::NotifyGroupFiltersChanged() - and deriving these again alongside
// it would be re-deriving an answer that could not have moved.
//
// Present on every space object, including the deep space ones SGP4Initialise() declines to
// initialise. Those carry SGP4Method::DeepSpace and a zeroed coefficient block, so why an object
// is not being propagated is readable off the object rather than inferred from what it is missing.
class SGP4Component : public IComponent
{
public:
    SGP4Component() = default;
    ~SGP4Component() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    SGP4ElementSet m_ElementSet;
};

REGISTER_COMPONENT(SGP4Component, "sgp4")

} // namespace WingsOfSteel
