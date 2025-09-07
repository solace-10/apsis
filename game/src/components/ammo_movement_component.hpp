#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class AmmoMovementComponent : public IComponent
{
public:
    AmmoMovementComponent() = default;
    ~AmmoMovementComponent() = default;

    float GetRange() const { return m_Range; }
    void SetRange(float value) { m_Range = value; }
    float GetSpeed() const { return m_Speed; }

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["range"] = m_Range;
        json["speed"] = m_Speed;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_Range = DeserializeOptional<float>(json, "range", 100.0f);
        m_Speed = DeserializeRequired<float>(json, "speed");
    }

private:
    float m_Range{ 100.0f };
    float m_Speed{ 10.0f };
};

REGISTER_COMPONENT(AmmoMovementComponent, "ammo_movement")

} // namespace WingsOfSteel