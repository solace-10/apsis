#pragma once

#include <glm/mat4x4.hpp>

#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class WeaponComponent
{
public:
    WeaponComponent() = default;

    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    float m_AngleDegrees{ 0.0f };
    float m_Range{ 100.0f };
    Pandora::EntityWeakPtr m_pParent;
};

} // namespace WingsOfSteel::TheBrightestStar