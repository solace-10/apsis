#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class AmmoRaycastComponent : public Pandora::IComponent
{
public:
    AmmoRaycastComponent() = default;
    ~AmmoRaycastComponent() = default;

    float GetRaycastLength() const { return m_RaycastLength; }

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["raycast_length"] = m_RaycastLength;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_RaycastLength = DeserializeRequired<float>(json, "raycast_length");
    }

private:
    float m_RaycastLength{ 5.0f };
    float m_Speed{ 10.0f };
};

REGISTER_COMPONENT(AmmoRaycastComponent, "ammo_raycast")

} // namespace WingsOfSteel::TheBrightestStar