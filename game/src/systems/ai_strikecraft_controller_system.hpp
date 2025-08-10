#pragma once

#include <glm/vec2.hpp>

#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class AIStrikecraftControllerSystem : public Pandora::System
{
public:
    AIStrikecraftControllerSystem() = default;
    ~AIStrikecraftControllerSystem() = default;

    void Initialize(Pandora::Scene* pScene) override {}
    void Update(float delta) override;

private:
    Pandora::EntitySharedPtr AcquireTarget() const;
};

} // namespace WingsOfSteel::TheBrightestStar