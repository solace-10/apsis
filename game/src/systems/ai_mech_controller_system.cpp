#include "systems/ai_mech_controller_system.hpp"
#include <glm/vec3.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

// Context definitions
struct MechNavigationContext
{
    EntityWeakPtr pTarget;
};

struct MechOffenseContext
{
    EntityWeakPtr pTarget;
};

struct MechDefenseContext
{
    bool underFire{ false };
};

// Navigation state implementations
class IdleNavigationState : public State<MechNavigationState, MechNavigationContext>
{
public:
    void OnEnter(MechNavigationContext& context) override
    {
    }

    void Update(float delta, MechNavigationContext& context) override
    {
    }

    MechNavigationState GetStateID() const override { return MechNavigationState::Idle; }
};

class EngageTargetNavigationState : public State<MechNavigationState, MechNavigationContext>
{
public:
    void OnEnter(MechNavigationContext& context) override
    {
    }

    void Update(float delta, MechNavigationContext& context) override
    {
    }

    MechNavigationState GetStateID() const override { return MechNavigationState::EngageTarget; }
};

// Offense state implementations
class IdleOffenseState : public State<MechOffenseState, MechOffenseContext>
{
public:
    void Update(float delta, MechOffenseContext& context) override
    {
    }

    MechOffenseState GetStateID() const override { return MechOffenseState::Idle; }
};

class AttackOffenseState : public State<MechOffenseState, MechOffenseContext>
{
public:
    void Update(float delta, MechOffenseContext& context) override
    {
        // Attack target
    }

    MechOffenseState GetStateID() const override { return MechOffenseState::Attack; }
};

// Defense state implementations
class IdleDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    void Update(float delta, MechDefenseContext& context) override
    {
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::Idle; }
};

class UnderFireDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    void OnEnter(MechDefenseContext& context) override
    {
    }

    void Update(float delta, MechDefenseContext& context) override
    {
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::UnderFire; }
};

class RetreatDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    void OnEnter(MechDefenseContext& context) override
    {
    }

    void Update(float delta, MechDefenseContext& context) override
    {
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::Retreat; }
};

void AIMechControllerSystem::Initialize(Scene* pScene)
{
    // Create and configure Navigation state machine
    m_pNavigationStateMachine = std::make_unique<StateMachine<MechNavigationState, MechNavigationContext>>();
    m_pNavigationStateMachine->AddState(MechNavigationState::Idle, std::make_unique<IdleNavigationState>());
    m_pNavigationStateMachine->AddState(MechNavigationState::EngageTarget, std::make_unique<EngageTargetNavigationState>());

    // Define valid transitions for Navigation
    m_pNavigationStateMachine->AddTransitions(MechNavigationState::Idle, { MechNavigationState::EngageTarget });
    m_pNavigationStateMachine->AddTransitions(MechNavigationState::EngageTarget, { MechNavigationState::Idle });

    m_pNavigationStateMachine->SetInitialState(MechNavigationState::Idle);

    // Create and configure Offense state machine
    m_pOffenseStateMachine = std::make_unique<StateMachine<MechOffenseState, MechOffenseContext>>();
    m_pOffenseStateMachine->AddState(MechOffenseState::Idle, std::make_unique<IdleOffenseState>());
    m_pOffenseStateMachine->AddState(MechOffenseState::Attack, std::make_unique<AttackOffenseState>());

    // Define valid transitions for Offense
    m_pOffenseStateMachine->AddTransitions(MechOffenseState::Idle, { MechOffenseState::Attack });
    m_pOffenseStateMachine->AddTransitions(MechOffenseState::Attack, { MechOffenseState::Idle });

    m_pOffenseStateMachine->SetInitialState(MechOffenseState::Idle);

    // Create and configure Defense state machine
    m_pDefenseStateMachine = std::make_unique<StateMachine<MechDefenseState, MechDefenseContext>>();
    m_pDefenseStateMachine->AddState(MechDefenseState::Idle, std::make_unique<IdleDefenseState>());
    m_pDefenseStateMachine->AddState(MechDefenseState::UnderFire, std::make_unique<UnderFireDefenseState>());
    m_pDefenseStateMachine->AddState(MechDefenseState::Retreat, std::make_unique<RetreatDefenseState>());

    // Define valid transitions for Defense
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::Idle, { MechDefenseState::UnderFire });
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::UnderFire, { MechDefenseState::Idle, MechDefenseState::Retreat });
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::Retreat, { MechDefenseState::Idle });

    m_pDefenseStateMachine->SetInitialState(MechDefenseState::Idle);
}

void AIMechControllerSystem::Update(float delta)
{
    // TODO: iterate through all the AIMechControllerComponents
    // Each component should have contexts for all state machines.
    // For now, this demonstrates how to use the state machines:

    // Example usage (would be per-entity in actual implementation):
    // MechNavigationContext navContext;
    // m_pNavigationStateMachine->Update(delta, navContext);
    // m_pNavigationStateMachine->TransitionTo(MechNavigationState::EngageTarget, navContext);
}

} // namespace WingsOfSteel
