#pragma once

#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <core/log.hpp>
#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>
#include <pandora.hpp>

namespace WingsOfSteel
{

class ShieldComponent : public IComponent
{
public:
    ShieldComponent() = default;

    // Owner: the Entity that has this component.
    void SetOwner(EntityWeakPtr pOwner) { m_pOwner = pOwner; }
    EntityWeakPtr GetOwner() { return m_pOwner; }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

private:
    EntityWeakPtr m_pOwner;
};

REGISTER_COMPONENT(ShieldComponent, "shield")

} // namespace WingsOfSteel
