#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include <scene/entity.hpp>
#include <scene/state_machine/state_machine.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class WingComponent;

// Forward declaration
enum class StrikecraftState;

// Context for the strikecraft state machine
struct StrikecraftContext
{
    EntityWeakPtr pOwner;
    EntityWeakPtr pTarget;

    // Combat parameters (copied from component on init)
    float minRange{ 100.0f };
    float maxRange{ 800.0f };
    float firingAngle{ 15.0f };
    float attackDuration{ 3.0f };
    float breakDuration{ 2.0f };

    // State-specific data
    float stateTimer{ 0.0f };
    glm::vec3 breakDirection{ 1.0f, 0.0f, 0.0f };
    glm::vec3 repositionTarget{ 0.0f };

    std::optional<StrikecraftState> currentState;
};

enum class StrikecraftState
{
    LaunchingFromCarrier,
    Approach,
    Attack,
    Break,
    Reposition
};

class AIStrikecraftControllerSystem : public System
{
public:
    AIStrikecraftControllerSystem() = default;
    ~AIStrikecraftControllerSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

private:
    EntitySharedPtr AcquireTarget(EntitySharedPtr pAcquiringEntity, const WingComponent& wingComponent) const;

    std::unique_ptr<StateMachine<StrikecraftState, StrikecraftContext>> m_pStateMachine;
};

} // namespace WingsOfSteel
