#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class AIStrategicSystem : public System
{
public:
    AIStrategicSystem() = default;
    ~AIStrategicSystem() = default;

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

    void DrawDebugUI();
    bool IsDebugUIVisible() const { return m_ShowDebugUI; }
    void ShowDebugUI(bool state) { m_ShowDebugUI = state; }

private:
    float m_NextUpdate{0.0f};
    bool m_ShowDebugUI{false};
};

} // namespace WingsOfSteel