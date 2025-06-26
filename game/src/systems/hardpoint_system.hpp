#pragma once

#include <glm/vec3.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class HardpointSystem : public Pandora::System
{
public:
    HardpointSystem() = default;
    ~HardpointSystem();

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

private:
    void DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength);
};

} // namespace WingsOfSteel::TheBrightestStar
