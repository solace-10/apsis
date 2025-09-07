#pragma once

#include "scene/components/component_factory.hpp"
#include "scene/components/icomponent.hpp"
#include "scene/entity.hpp"

namespace WingsOfSteel
{

class EntityReferenceComponent : public IComponent
{
public:
    EntityReferenceComponent() = default;
    ~EntityReferenceComponent() = default;

    EntityReferenceComponent(EntitySharedPtr pOwner)
    : m_pOwner(pOwner)
    {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

    void SetOwner(EntitySharedPtr pEntity) { m_pOwner = pEntity; }
    EntitySharedPtr GetOwner() const { return m_pOwner.lock(); }

private:
    EntityWeakPtr m_pOwner;
};

REGISTER_COMPONENT(EntityReferenceComponent, "entity_reference")

} // namespace WingsOfSteel