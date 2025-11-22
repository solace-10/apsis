#pragma once

#include <scene/state_machine/state_machine.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

// Forward declarations for state machine contexts
struct MechNavigationContext;
struct MechOffenseContext;
struct MechDefenseContext;

enum class MechNavigationState
{
    Idle,
    EngageTarget
};

enum class MechOffenseState
{
    Idle,
    Attack
};

enum class MechDefenseState
{
    Idle,
    UnderFire,
    Retreat
};

class AIMechControllerSystem : public System
{
public:
    AIMechControllerSystem() = default;
    ~AIMechControllerSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

private:
    std::unique_ptr<StateMachine<MechNavigationState, MechNavigationContext>> m_pNavigationStateMachine;
    std::unique_ptr<StateMachine<MechOffenseState, MechOffenseContext>> m_pOffenseStateMachine;
    std::unique_ptr<StateMachine<MechDefenseState, MechDefenseContext>> m_pDefenseStateMachine;
};

} // namespace WingsOfSteel
