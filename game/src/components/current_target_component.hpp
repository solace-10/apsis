#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

class CurrentTargetComponent : public IComponent
{
public:
    CurrentTargetComponent() = default;
    ~CurrentTargetComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        // No deserialization needed for runtime-only component
    }

    void SetTarget(EntitySharedPtr pEntity) { m_pTarget = pEntity; }
    EntitySharedPtr GetTarget() const { return m_pTarget.lock(); }

private:
    EntityWeakPtr m_pTarget;
};

REGISTER_COMPONENT(CurrentTargetComponent, "current_target")

} // namespace WingsOfSteel
