#include <core/random.hpp>
#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/ai_strikecraft_controller_component.hpp"
#include "components/hardpoint_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/weapon_component.hpp"
#include "components/wing_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/ai_strikecraft_controller_system.hpp"
#include "systems/ai_utils.hpp"

namespace WingsOfSteel
{

// Helper functions (file-local)
namespace
{
    glm::vec3 CalculateInterceptPoint(const glm::vec3& shooterPos, const glm::vec3& targetPos, const glm::vec3& targetVel, float projectileSpeed)
    {
        const glm::vec3 toTarget = targetPos - shooterPos;
        const float distance = glm::length(toTarget);
        const float timeToIntercept = distance / projectileSpeed;
        return targetPos + targetVel * timeToIntercept;
    }

    glm::vec3 GenerateBreakDirection(const glm::vec3& forward, const glm::vec3& toTarget)
    {
        const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        const float rightComponent = Random::Get(-1.0f, 1.0f);
        const float forwardComponent = Random::Get(-0.5f, 0.5f);
        const glm::vec3 breakDir = right * rightComponent + forward * forwardComponent;
        return glm::normalize(breakDir);
    }

    void UpdateWeaponSystems(EntitySharedPtr pOwner, const glm::vec3& targetPos, bool shouldFire)
    {
        if (!pOwner || !pOwner->HasComponent<HardpointComponent>())
        {
            return;
        }

        HardpointComponent& hardpointComponent = pOwner->GetComponent<HardpointComponent>();
        for (const auto& hardpoint : hardpointComponent.hardpoints)
        {
            if (hardpoint.m_pEntity && hardpoint.m_pEntity->HasComponent<WeaponComponent>())
            {
                WeaponComponent& weaponComponent = hardpoint.m_pEntity->GetComponent<WeaponComponent>();
                weaponComponent.m_TargetPosition = targetPos;
                weaponComponent.m_WantsToFire = shouldFire;
            }
        }
    }

    void CeaseFire(EntitySharedPtr pOwner)
    {
        if (!pOwner || !pOwner->HasComponent<HardpointComponent>())
        {
            return;
        }

        HardpointComponent& hardpointComponent = pOwner->GetComponent<HardpointComponent>();
        for (const auto& hardpoint : hardpointComponent.hardpoints)
        {
            if (hardpoint.m_pEntity && hardpoint.m_pEntity->HasComponent<WeaponComponent>())
            {
                WeaponComponent& weaponComponent = hardpoint.m_pEntity->GetComponent<WeaponComponent>();
                weaponComponent.m_WantsToFire = false;
            }
        }
    }
} // anonymous namespace

class LaunchingFromCarrierState : public State<StrikecraftState, StrikecraftContext>
{
public:
    StrikecraftState GetStateID() const override { return StrikecraftState::LaunchingFromCarrier; }

    std::optional<StrikecraftState> Update(float delta, StrikecraftContext& context) override
    {
        // The carrier launch system will transition this state externally
        // when the strikecraft has cleared the hangar.
        // This state is a placeholder - the actual transition happens via
        // external code setting context.currentState to Approach.
        return std::nullopt;
    }
};

class ApproachState : public State<StrikecraftState, StrikecraftContext>
{
public:
    StrikecraftState GetStateID() const override { return StrikecraftState::Approach; }

    void OnEnter(StrikecraftContext& context) override
    {
        context.stateTimer = 0.0f;
    }

    std::optional<StrikecraftState> Update(float delta, StrikecraftContext& context) override
    {
        EntitySharedPtr pOwner = context.pOwner.lock();
        EntitySharedPtr pTarget = context.pTarget.lock();
        if (!pOwner || !pTarget)
        {
            return std::nullopt;
        }

        const auto& transform = pOwner->GetComponent<TransformComponent>();
        auto& navigation = pOwner->GetComponent<ShipNavigationComponent>();

        const glm::vec3 strikecraftPosition = transform.GetTranslation();
        const glm::vec3 targetPosition = pTarget->GetComponent<TransformComponent>().GetTranslation();

        // Navigate toward intercept point
        glm::vec3 interceptPoint = CalculateInterceptPoint(strikecraftPosition, targetPosition, glm::vec3(0.0f), 1000.0f);
        navigation.SetTarget(interceptPoint);
        navigation.SetThrust(ShipThrust::Forward);

        // Weapons off during approach
        UpdateWeaponSystems(pOwner, targetPosition, false);

        // Transition to Attack when in range
        const float distanceToTarget = glm::length(targetPosition - strikecraftPosition);
        if (distanceToTarget <= context.maxRange)
        {
            return StrikecraftState::Attack;
        }

        return std::nullopt;
    }
};

class AttackState : public State<StrikecraftState, StrikecraftContext>
{
public:
    StrikecraftState GetStateID() const override { return StrikecraftState::Attack; }

    void OnEnter(StrikecraftContext& context) override
    {
        context.stateTimer = 0.0f;
    }

    std::optional<StrikecraftState> Update(float delta, StrikecraftContext& context) override
    {
        context.stateTimer += delta;

        EntitySharedPtr pOwner = context.pOwner.lock();
        EntitySharedPtr pTarget = context.pTarget.lock();
        if (!pOwner || !pTarget)
        {
            return std::nullopt;
        }

        const auto& transform = pOwner->GetComponent<TransformComponent>();
        auto& navigation = pOwner->GetComponent<ShipNavigationComponent>();

        const glm::vec3 myPos = transform.GetTranslation();
        const glm::vec3 targetPos = pTarget->GetComponent<TransformComponent>().GetTranslation();
        const glm::vec3 toTarget = targetPos - myPos;
        const float distanceToTarget = glm::length(toTarget);
        const glm::vec3 forward = transform.GetForward();
        const float angleToTarget = glm::degrees(glm::acos(glm::clamp(glm::dot(glm::normalize(toTarget), forward), -1.0f, 1.0f)));

        // Navigate toward intercept point
        glm::vec3 interceptPoint = CalculateInterceptPoint(myPos, targetPos, glm::vec3(0.0f), 1000.0f);
        navigation.SetTarget(interceptPoint);
        navigation.SetThrust(ShipThrust::Forward);

        // Fire if conditions are met
        const bool shouldFire = distanceToTarget >= context.minRange && distanceToTarget <= context.maxRange && angleToTarget <= context.firingAngle;

        UpdateWeaponSystems(pOwner, targetPos, shouldFire);

        // Transition to Break when attack duration expires or too close
        if (context.stateTimer >= context.attackDuration || distanceToTarget < context.minRange)
        {
            // Pre-calculate break direction for the Break state
            context.breakDirection = GenerateBreakDirection(forward, glm::normalize(toTarget));
            return StrikecraftState::Break;
        }

        return std::nullopt;
    }

    void OnExit(StrikecraftContext& context) override
    {
        // Cease fire when leaving attack state
        EntitySharedPtr pOwner = context.pOwner.lock();
        if (pOwner)
        {
            CeaseFire(pOwner);
        }
    }
};

class BreakState : public State<StrikecraftState, StrikecraftContext>
{
public:
    StrikecraftState GetStateID() const override { return StrikecraftState::Break; }

    void OnEnter(StrikecraftContext& context) override
    {
        context.stateTimer = 0.0f;
    }

    std::optional<StrikecraftState> Update(float delta, StrikecraftContext& context) override
    {
        context.stateTimer += delta;

        EntitySharedPtr pOwner = context.pOwner.lock();
        EntitySharedPtr pTarget = context.pTarget.lock();
        if (!pOwner)
        {
            return std::nullopt;
        }

        const auto& transform = pOwner->GetComponent<TransformComponent>();
        auto& navigation = pOwner->GetComponent<ShipNavigationComponent>();

        const glm::vec3 myPos = transform.GetTranslation();

        // Break away from target
        glm::vec3 breakTarget = myPos + context.breakDirection * 100.0f;
        navigation.SetTarget(breakTarget);
        navigation.SetThrust(ShipThrust::Forward);

        // Weapons off during break
        if (pTarget)
        {
            const glm::vec3 targetPos = pTarget->GetComponent<TransformComponent>().GetTranslation();
            UpdateWeaponSystems(pOwner, targetPos, false);
        }

        // Transition after break duration
        if (context.stateTimer >= context.breakDuration)
        {
            if (Random::Get(0.0f, 1.0f) < 0.8f)
            {
                return StrikecraftState::Approach;
            }
            else
            {
                // Calculate reposition target
                if (pTarget)
                {
                    const glm::vec3 targetPos = pTarget->GetComponent<TransformComponent>().GetTranslation();
                    const glm::vec3 repositionDirection = glm::normalize(glm::vec3(Random::Get(-1.0f, 1.0f), 0.0f, Random::Get(-1.0f, 1.0f)));
                    context.repositionTarget = targetPos + repositionDirection * context.maxRange;
                }
                return StrikecraftState::Reposition;
            }
        }

        return std::nullopt;
    }
};

class RepositionState : public State<StrikecraftState, StrikecraftContext>
{
public:
    StrikecraftState GetStateID() const override { return StrikecraftState::Reposition; }

    void OnEnter(StrikecraftContext& context) override
    {
        context.stateTimer = 0.0f;
    }

    void OnExit(StrikecraftContext& context) override
    {
        context.pTarget.reset();
    }

    std::optional<StrikecraftState> Update(float delta, StrikecraftContext& context) override
    {
        EntitySharedPtr pOwner = context.pOwner.lock();
        if (!pOwner)
        {
            return std::nullopt;
        }

        const auto& transform = pOwner->GetComponent<TransformComponent>();
        auto& navigation = pOwner->GetComponent<ShipNavigationComponent>();

        const glm::vec3 myPos = transform.GetTranslation();

        // Navigate to reposition target
        navigation.SetTarget(context.repositionTarget);
        navigation.SetThrust(ShipThrust::Forward);

        // Weapons off during reposition
        EntitySharedPtr pTarget = context.pTarget.lock();
        if (pTarget)
        {
            const glm::vec3 targetPos = pTarget->GetComponent<TransformComponent>().GetTranslation();
            UpdateWeaponSystems(pOwner, targetPos, false);
        }

        // Transition to Approach when near reposition target
        const float repositionGoalRadius = 20.0f;
        const float distanceToReposition = glm::length(context.repositionTarget - myPos);
        if (distanceToReposition < repositionGoalRadius)
        {
            return StrikecraftState::Approach;
        }

        return std::nullopt;
    }
};

void AIStrikecraftControllerSystem::Initialize(Scene* pScene)
{
    // Create state machine
    m_pStateMachine = std::make_unique<StateMachine<StrikecraftState, StrikecraftContext>>();

    // Register states
    m_pStateMachine->AddState(StrikecraftState::LaunchingFromCarrier, std::make_unique<LaunchingFromCarrierState>());
    m_pStateMachine->AddState(StrikecraftState::Approach, std::make_unique<ApproachState>());
    m_pStateMachine->AddState(StrikecraftState::Attack, std::make_unique<AttackState>());
    m_pStateMachine->AddState(StrikecraftState::Break, std::make_unique<BreakState>());
    m_pStateMachine->AddState(StrikecraftState::Reposition, std::make_unique<RepositionState>());

    // Set initial state
    m_pStateMachine->SetInitialState(StrikecraftState::LaunchingFromCarrier);

    // Define transitions
    m_pStateMachine->AddTransitions(StrikecraftState::LaunchingFromCarrier, { StrikecraftState::Approach });
    m_pStateMachine->AddTransitions(StrikecraftState::Approach, { StrikecraftState::Attack });
    m_pStateMachine->AddTransitions(StrikecraftState::Attack, { StrikecraftState::Break });
    m_pStateMachine->AddTransitions(StrikecraftState::Break, { StrikecraftState::Approach, StrikecraftState::Reposition });
    m_pStateMachine->AddTransitions(StrikecraftState::Reposition, { StrikecraftState::Approach });
}

void AIStrikecraftControllerSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<AIStrikecraftControllerComponent, const WingComponent>();

    view.each([this, delta, &registry](const auto entityHandle, AIStrikecraftControllerComponent& controller, const WingComponent& wingComponent) {
        StrikecraftContext& context = controller.Context;

        // Skip if still launching (handled externally by carrier system)
        if (context.currentState.has_value() && context.currentState.value() == StrikecraftState::LaunchingFromCarrier)
        {
            return;
        }

        EntitySharedPtr pOwner = context.pOwner.lock();
        if (!pOwner)
        {
            return;
        }

        // Acquire target if needed
        EntitySharedPtr pTarget = context.pTarget.lock();
        if (!pTarget)
        {
            pTarget = AcquireTarget(pOwner, wingComponent);
            context.pTarget = pTarget;
        }

        // Update state machine if we have a target
        if (pTarget)
        {
            m_pStateMachine->Update(delta, context);
        }
        else
        {
            // No target - clear navigation
            if (pOwner->HasComponent<ShipNavigationComponent>())
            {
                auto& navigation = pOwner->GetComponent<ShipNavigationComponent>();
                navigation.ClearTarget();
                navigation.SetThrust(ShipThrust::None);
            }
        }
    });
}

EntitySharedPtr AIStrikecraftControllerSystem::AcquireTarget(EntitySharedPtr pAcquiringEntity, const WingComponent& wingComponent) const
{
    const WingRole role = wingComponent.Role;
    if (role == WingRole::Offense)
    {
        return AIUtils::AcquireTarget(pAcquiringEntity, { ThreatCategory::Carrier, ThreatCategory::AntiCapital, ThreatCategory::Interceptor }, AIUtils::TargetRangeOrder::Closest);
    }
    else if (role == WingRole::Interception || role == WingRole::Defense)
    {
        return AIUtils::AcquireTarget(pAcquiringEntity, { ThreatCategory::AntiCapital, ThreatCategory::Carrier, ThreatCategory::Interceptor }, AIUtils::TargetRangeOrder::Closest);
    }
    return nullptr;
}

} // namespace WingsOfSteel
