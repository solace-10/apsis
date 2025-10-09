#pragma once

#include <imgui/idebugui.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class AIStrategicSystem : public System, public IDebugUI
{
public:
    AIStrategicSystem() = default;
    ~AIStrategicSystem() = default;

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

    void DrawDebugUI() override;

private:
    float m_NextUpdate{0.0f};
};

} // namespace WingsOfSteel