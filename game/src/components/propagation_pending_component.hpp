#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

// Marks a visible object that no readback has placed yet, so that nothing draws it, picks it or
// reports it at a position it does not have. Space objects are created with a default
// TransformComponent and nothing seeds a position into it, so until then an object is at the world
// origin - or, if it has been visible before, wherever it was when it was last on screen.
//
// Only ever exists alongside an OrbitalStateComponent: the two are emplaced together when an object
// becomes visible and removed together when it stops being. Consumers exclude it from their views
// rather than testing it per object. It is cleared by a readback actually placing the object, so an
// object whose propagation fails keeps it and is never drawn at all.
class PropagationPendingComponent : public IComponent
{
public:
    PropagationPendingComponent() = default;
    ~PropagationPendingComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }
};

REGISTER_COMPONENT(PropagationPendingComponent, "propagation_pending")

} // namespace WingsOfSteel
