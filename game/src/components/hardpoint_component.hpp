#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel::TheBrightestStar
{

struct Hardpoint
{
    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    float m_AngleDegrees{ 0.0f };
    Pandora::EntitySharedPtr m_pEntity;
    Pandora::EntityWeakPtr m_pParent;
};

class HardpointComponent
{
public:
    HardpointComponent() {}
    std::vector<Hardpoint> hardpoints;
};

} // namespace WingsOfSteel::TheBrightestStar