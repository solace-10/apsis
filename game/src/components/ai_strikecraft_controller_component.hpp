#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class AIStrikecraftControllerComponent : public Pandora::IComponent
{
public:
    AIStrikecraftControllerComponent() {}
    ~AIStrikecraftControllerComponent() {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

    void SetTarget(Pandora::EntitySharedPtr pEntity) { m_pTarget = pEntity; }
    Pandora::EntitySharedPtr GetTarget() const { return m_pTarget.lock(); }

private:
    Pandora::EntityWeakPtr m_pTarget;
};

REGISTER_COMPONENT(AIStrikecraftControllerComponent, "ai_strikecraft_controller")

} // namespace WingsOfSteel::TheBrightestStar