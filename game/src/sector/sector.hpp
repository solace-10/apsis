#pragma once

#include <glm/vec3.hpp>

#include <core/signal.hpp>
#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

#include "components/wing_component.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(Wing);

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

    Encounter* GetEncounter();
    EntitySharedPtr GetPlayerMech() const;
    EntitySharedPtr GetPlayerCarrier() const;

    Signal<EntitySharedPtr, EntitySharedPtr>& GetEntityKilledSignal() { return m_EntityKilledSignal; }

private:
    void DrawCameraDebugUI();
    void SpawnDome();
    void SpawnLight();
    void SpawnPlayerFleet();
    void SpawnMech(const glm::vec3& position, float angle, bool isPlayerMech, WingRole role);

    EncounterUniquePtr m_pEncounter;
    EntitySharedPtr m_pDome;
    EntitySharedPtr m_pCamera;
    EntitySharedPtr m_pLight;
    EntityWeakPtr m_pPlayerMech;
    EntityWeakPtr m_pCarrier;
    bool m_ShowCameraDebugUI{ false };
    bool m_ShowGrid{ false };

    Signal<EntitySharedPtr, EntitySharedPtr> m_EntityKilledSignal;
};

inline Encounter* Sector::GetEncounter()
{
    return m_pEncounter.get();
}

inline EntitySharedPtr Sector::GetPlayerMech() const
{
    return m_pPlayerMech.lock();
}

inline EntitySharedPtr Sector::GetPlayerCarrier() const
{
    return m_pCarrier.lock();
}

} // namespace WingsOfSteel
