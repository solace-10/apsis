#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

using SpaceObjectGroupId = uint16_t;

class SpaceObjectGroupComponent : public IComponent
{
public:
    SpaceObjectGroupComponent() = default;

    SpaceObjectGroupComponent(SpaceObjectGroupId groupId)
        : m_GroupId(groupId)
    {
    }

    ~SpaceObjectGroupComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    SpaceObjectGroupId GetGroupId() const { return m_GroupId; }

private:
    SpaceObjectGroupId m_GroupId{ 0 };
};

REGISTER_COMPONENT(SpaceObjectGroupComponent, "space_object_group")

} // namespace WingsOfSteel
