#pragma once

#include <glm/vec3.hpp>

#include <core/signal.hpp>
#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Sector);
class Sector : public Scene
{
public:
    Sector();
    ~Sector();

    void Initialize() override;
    void Update(float delta) override;

    void ShowCameraDebugUI(bool state);
    void ShowGrid(bool state);

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnLight();

    EntitySharedPtr m_pDome;
    EntitySharedPtr m_pCamera;
    EntitySharedPtr m_pLight;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ false };
};

} // namespace WingsOfSteel
