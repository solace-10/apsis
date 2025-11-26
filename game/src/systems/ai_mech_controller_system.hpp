#pragma once

#include <optional>

#include <scene/entity.hpp>
#include <scene/state_machine/state_machine.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

// Forward declarations for state enums (defined below)
enum class MechNavigationState;
enum class MechOffenseState;
enum class MechDefenseState;

// Context definitions
struct MechNavigationContext
{
    EntityWeakPtr pOwner;
    float optimalRange{ 0.0f };
    std::optional<MechNavigationState> currentState;
};

struct MechOffenseContext
{
    EntityWeakPtr pOwner;
    float timeUntilRetarget{ 0.0f };
    float optimalRange{ 0.0f };
    std::optional<MechOffenseState> currentState;
};

struct MechDefenseContext
{
    bool underFire{ false };
    std::optional<MechDefenseState> currentState;
};

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
