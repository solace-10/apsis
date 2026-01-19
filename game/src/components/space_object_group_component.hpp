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

    SpaceObjectGroupComponent(SpaceObjectGroupId groupId, bool isPrimaryElement)
        : m_GroupId(groupId)
        , m_IsPrimaryElement(isPrimaryElement)
    {
    }

    ~SpaceObjectGroupComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    SpaceObjectGroupId GetGroupId() const { return m_GroupId; }
    bool IsPrimaryElement() const { return m_IsPrimaryElement; }

private:
    SpaceObjectGroupId m_GroupId{ 0 };
    bool m_IsPrimaryElement{ false };
};

REGISTER_COMPONENT(SpaceObjectGroupComponent, "space_object_group")

} // namespace WingsOfSteel
