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

enum class ShieldState
{
    Inactive,
    PoweringUp,
    Active,
    PoweringDown  
};

class ShieldComponent : public IComponent
{
public:
    ShieldComponent() = default;

    void SetOwner(EntityWeakPtr pOwner) { m_pOwner = pOwner; }
    EntityWeakPtr GetOwner() { return m_pOwner; }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& data) override
    {
        PowerUpDuration = Json::DeserializeFloat(pContext, data, "power_up_duration", 0.0f);
        PowerDownDuration = Json::DeserializeFloat(pContext, data, "power_down_duration", 0.0f);
    }

    ShieldState CurrentState{ ShieldState::Inactive };
    ShieldState WantedState{ ShieldState::Inactive };
    float TransitionValue{ 0.0f };
    float PowerUpDuration{ 0.0f };
    float PowerDownDuration{ 0.0f };

private:
    EntityWeakPtr m_pOwner;
};

REGISTER_COMPONENT(ShieldComponent, "shield")

} // namespace WingsOfSteel
