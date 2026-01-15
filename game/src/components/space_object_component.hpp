#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

#include "space_objects/space_object.hpp"

namespace WingsOfSteel
{

class SpaceObjectComponent : public IComponent
{
public:
    SpaceObjectComponent() {}
    ~SpaceObjectComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    void AssignSpaceObject(const SpaceObject& spaceObject) { m_SpaceObject = spaceObject; }
    const SpaceObject& GetSpaceObject() const { return m_SpaceObject; }
    SpaceObject& GetSpaceObject() { return m_SpaceObject; }

private:
    SpaceObject m_SpaceObject;
};

REGISTER_COMPONENT(SpaceObjectComponent, "space_object")

} // namespace WingsOfSteel
