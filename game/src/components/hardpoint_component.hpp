#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource_model.hpp>
#include <scene/entity.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

// Hardpoints represent where weapons can be attached to, as well as how
// far the weapons can rotate.
// There is no in-built distinction between turrets and fixed weapons: a
// fixed weapon is just a weapon attached to a hardpoint with a minimum
// and maximum arc of 0 degrees.
struct Hardpoint
{
    std::string m_Name;
    std::string m_Weapon; // Weapon to equip once this hardpoint is deserialised.
    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    Pandora::EntitySharedPtr m_pEntity;
    Pandora::EntityWeakPtr m_pParent;
    
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& json);
};

class HardpointComponent : public Pandora::IComponent
{
public:
    HardpointComponent() = default;
    ~HardpointComponent() = default;
    std::vector<Hardpoint> hardpoints;

    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& json) override;

private:
    Pandora::ResourceModelSharedPtr m_pResource;
    std::string m_ResourcePath;
};
REGISTER_COMPONENT(HardpointComponent, "hardpoint")

} // namespace WingsOfSteel::TheBrightestStar