
#include <glm/vec3.hpp>

#include <core/random.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

#include "components/ai_mech_controller_component.hpp"
#include "components/current_target_component.hpp"
#include "components/hardpoint_component.hpp"
#include "components/mech_engine_component.hpp"
#include "components/mech_navigation_component.hpp"
#include "components/threat_component.hpp"
#include "components/weapon_component.hpp"
#include "components/wing_component.hpp"
#include "systems/ai_mech_controller_system.hpp"
#include "systems/ai_utils.hpp"

#pragma optimize("", off)

namespace WingsOfSteel
{

// Navigation state implementations
class IdleNavigationState : public State<MechNavigationState, MechNavigationContext>
{
public:
    void OnEnter(MechNavigationContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (pMechEntity)
        {
            context.optimalRange = AIUtils::CalculateOptimalRange(pMechEntity);
        }
    }

    std::optional<MechNavigationState> Update(float delta, MechNavigationContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (!pMechEntity || !pMechEntity->HasComponent<CurrentTargetComponent>())
        {
            return std::nullopt;
        }

        const CurrentTargetComponent& currentTargetComponent = pMechEntity->GetComponent<CurrentTargetComponent>();
        if (currentTargetComponent.GetTarget())
        {
            return MechNavigationState::EngageTarget;
        }

        if (!pMechEntity->HasComponent<MechNavigationComponent>())
        {
            return std::nullopt;
        }

        pMechEntity->GetComponent<MechNavigationComponent>().ClearThrust();
        return std::nullopt;
    }

    MechNavigationState GetStateID() const override { return MechNavigationState::Idle; }
};

class EngageTargetNavigationState : public State<MechNavigationState, MechNavigationContext>
{
public:
    std::optional<MechNavigationState> Update(float delta, MechNavigationContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (!pMechEntity)
        {
            return std::nullopt;
        }

        if (!pMechEntity->HasComponent<CurrentTargetComponent>() || !pMechEntity->HasComponent<MechNavigationComponent>())
        {
            return MechNavigationState::Idle;
        }

        const CurrentTargetComponent& currentTargetComponent = pMechEntity->GetComponent<CurrentTargetComponent>();
        EntitySharedPtr pCurrentTarget = currentTargetComponent.GetTarget();
        if (!pCurrentTarget)
        {
            return MechNavigationState::Idle;
        }

        const glm::vec3& mechPosition = pMechEntity->GetComponent<TransformComponent>().GetTranslation();
        const glm::vec3& targetPosition = pCurrentTarget->GetComponent<TransformComponent>().GetTranslation();
        const float distanceToTarget = glm::length(targetPosition - mechPosition);
        const glm::vec3 directionToTarget = glm::normalize(targetPosition - mechPosition);
        MechNavigationComponent& mechNavigationComponent = pMechEntity->GetComponent<MechNavigationComponent>();
        if (distanceToTarget > context.optimalRange)
        {
            mechNavigationComponent.SetThrust(directionToTarget);
        }
        else if (distanceToTarget < context.optimalRange * 0.75f)
        {
            mechNavigationComponent.SetThrust(-directionToTarget);
        }
        else
        {
            mechNavigationComponent.ClearThrust();
        }

        return std::nullopt;
    }

    MechNavigationState GetStateID() const override { return MechNavigationState::EngageTarget; }
};

// Offense state implementations
class AcquireTargetOffenseState : public State<MechOffenseState, MechOffenseContext>
{
public:
    std::optional<MechOffenseState> Update(float delta, MechOffenseContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (!pMechEntity)
        {
            return std::nullopt;
        }

        WingRole wingRole = WingRole::Interception;
        if (pMechEntity->HasComponent<WingComponent>())
        {
            wingRole = pMechEntity->GetComponent<WingComponent>().Role;
        }

        EntitySharedPtr pAcquiredTarget;
        if (wingRole == WingRole::Defense)
        {
            pAcquiredTarget = AIUtils::AcquireTarget(pMechEntity, { ThreatCategory::AntiCapital, ThreatCategory::Interceptor, ThreatCategory::Carrier }, AIUtils::TargetRangeOrder::ClosestToCarrier);
        }
        else
        {
            pAcquiredTarget = AIUtils::AcquireTarget(pMechEntity, { ThreatCategory::Interceptor, ThreatCategory::AntiCapital, ThreatCategory::Carrier }, AIUtils::TargetRangeOrder::Closest);
        }

        if (pAcquiredTarget)
        {
            context.timeUntilRetarget = Random::Get(3.0f, 5.0f);
            if (!pMechEntity->HasComponent<CurrentTargetComponent>())
            {
                pMechEntity->AddComponent<CurrentTargetComponent>();
            }

            pMechEntity->GetComponent<CurrentTargetComponent>().SetTarget(pAcquiredTarget);

            return MechOffenseState::Attack;
        }

        return std::nullopt;
    }

    MechOffenseState GetStateID() const override { return MechOffenseState::Idle; }
};

class AttackOffenseState : public State<MechOffenseState, MechOffenseContext>
{
public:
    void OnEnter(MechOffenseContext& context) override
    {
        if (context.optimalRange <= 0.0f)
        {
            EntitySharedPtr pMechEntity = context.pOwner.lock();
            if (pMechEntity)
            {
                context.optimalRange = AIUtils::CalculateOptimalRange(pMechEntity);
            }
        }
    }

    void OnExit(MechOffenseContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (!pMechEntity)
        {
            return;
        }

        CeaseFire(pMechEntity);
    }

    std::optional<MechOffenseState> Update(float delta, MechOffenseContext& context) override
    {
        EntitySharedPtr pMechEntity = context.pOwner.lock();
        if (!pMechEntity)
        {
            return std::nullopt;
        }

        context.timeUntilRetarget -= delta;
        if (context.timeUntilRetarget <= 0.0f)
        {
            context.timeUntilRetarget = Random::Get(3.0f, 5.0f);
            return MechOffenseState::Idle;
        }

        if (!pMechEntity->HasComponent<CurrentTargetComponent>())
        {
            return MechOffenseState::Idle;
        }

        CurrentTargetComponent& currentTargetComponent = pMechEntity->GetComponent<CurrentTargetComponent>();
        EntitySharedPtr pTargetEntity = currentTargetComponent.GetTarget();
        if (!pTargetEntity)
        {
            return MechOffenseState::Idle;
        }

        TransformComponent& targetTransformComponent = pTargetEntity->GetComponent<TransformComponent>();

        MechNavigationComponent& mechNavigationComponent = pMechEntity->GetComponent<MechNavigationComponent>();
        mechNavigationComponent.SetAim(targetTransformComponent.GetTranslation());

        // Fire if within weapon range.
        if (!pMechEntity->HasComponent<HardpointComponent>())
        {
            return std::nullopt;
        }

        const glm::vec3 targetPosition = targetTransformComponent.GetTranslation();
        const float distanceToTarget = glm::distance(pMechEntity->GetComponent<TransformComponent>().GetTranslation(), targetPosition);
        HardpointComponent& hardpointComponent = pMechEntity->GetComponent<HardpointComponent>();
        for (auto& hardpoint : hardpointComponent.hardpoints)
        {
            EntitySharedPtr pWeaponEntity = hardpoint.m_pEntity;
            if (!pWeaponEntity || !pWeaponEntity->HasComponent<WeaponComponent>())
            {
                continue;
            }

            WeaponComponent& weaponComponent = pWeaponEntity->GetComponent<WeaponComponent>();
            weaponComponent.m_WantsToFire = (distanceToTarget <= weaponComponent.m_Range);
            weaponComponent.m_TargetPosition = targetPosition;
        }

        return std::nullopt;
    }

    MechOffenseState GetStateID() const override { return MechOffenseState::Attack; }

private:
    void CeaseFire(EntitySharedPtr pMechEntity)
    {
        if (!pMechEntity->HasComponent<HardpointComponent>())
        {
            return;
        }

        HardpointComponent& hardpointComponent = pMechEntity->GetComponent<HardpointComponent>();
        for (auto& hardpoint : hardpointComponent.hardpoints)
        {
            EntitySharedPtr pWeaponEntity = hardpoint.m_pEntity;
            if (!pWeaponEntity || !pWeaponEntity->HasComponent<WeaponComponent>())
            {
                continue;
            }

            WeaponComponent& weaponComponent = pWeaponEntity->GetComponent<WeaponComponent>();
            weaponComponent.m_WantsToFire = false;
            weaponComponent.m_TargetPosition.reset();
        }
    }
};

// Defense state implementations
class IdleDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    std::optional<MechDefenseState> Update(float delta, MechDefenseContext& context) override
    {
        return std::nullopt;
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::Idle; }
};

class UnderFireDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    void OnEnter(MechDefenseContext& context) override
    {
    }

    std::optional<MechDefenseState> Update(float delta, MechDefenseContext& context) override
    {
        return std::nullopt;
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::UnderFire; }
};

class RetreatDefenseState : public State<MechDefenseState, MechDefenseContext>
{
public:
    void OnEnter(MechDefenseContext& context) override
    {
    }

    std::optional<MechDefenseState> Update(float delta, MechDefenseContext& context) override
    {
        return std::nullopt;
    }

    MechDefenseState GetStateID() const override { return MechDefenseState::Retreat; }
};

void AIMechControllerSystem::Initialize(Scene* pScene)
{
    // Create and configure Navigation state machine
    m_pNavigationStateMachine = std::make_unique<StateMachine<MechNavigationState, MechNavigationContext>>();
    m_pNavigationStateMachine->AddState(MechNavigationState::Idle, std::make_unique<IdleNavigationState>());
    m_pNavigationStateMachine->AddState(MechNavigationState::EngageTarget, std::make_unique<EngageTargetNavigationState>());
    m_pNavigationStateMachine->SetInitialState(MechNavigationState::Idle);

    // Define valid transitions for Navigation
    m_pNavigationStateMachine->AddTransitions(MechNavigationState::Idle, { MechNavigationState::EngageTarget });
    m_pNavigationStateMachine->AddTransitions(MechNavigationState::EngageTarget, { MechNavigationState::Idle });

    // Create and configure Offense state machine
    m_pOffenseStateMachine = std::make_unique<StateMachine<MechOffenseState, MechOffenseContext>>();
    m_pOffenseStateMachine->AddState(MechOffenseState::Idle, std::make_unique<AcquireTargetOffenseState>());
    m_pOffenseStateMachine->AddState(MechOffenseState::Attack, std::make_unique<AttackOffenseState>());
    m_pOffenseStateMachine->SetInitialState(MechOffenseState::Idle);

    // Define valid transitions for Offense
    m_pOffenseStateMachine->AddTransitions(MechOffenseState::Idle, { MechOffenseState::Attack });
    m_pOffenseStateMachine->AddTransitions(MechOffenseState::Attack, { MechOffenseState::Idle });

    // Create and configure Defense state machine
    m_pDefenseStateMachine = std::make_unique<StateMachine<MechDefenseState, MechDefenseContext>>();
    m_pDefenseStateMachine->AddState(MechDefenseState::Idle, std::make_unique<IdleDefenseState>());
    m_pDefenseStateMachine->AddState(MechDefenseState::UnderFire, std::make_unique<UnderFireDefenseState>());
    m_pDefenseStateMachine->AddState(MechDefenseState::Retreat, std::make_unique<RetreatDefenseState>());
    m_pDefenseStateMachine->SetInitialState(MechDefenseState::Idle);

    // Define valid transitions for Defense
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::Idle, { MechDefenseState::UnderFire });
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::UnderFire, { MechDefenseState::Idle, MechDefenseState::Retreat });
    m_pDefenseStateMachine->AddTransitions(MechDefenseState::Retreat, { MechDefenseState::Idle });
}

void AIMechControllerSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto view = registry.view<AIMechControllerComponent>();
    view.each([this, delta](const auto entityHandle, AIMechControllerComponent& mechControllerComponent) {
        m_pNavigationStateMachine->Update(delta, mechControllerComponent.NavigationContext);
        m_pOffenseStateMachine->Update(delta, mechControllerComponent.OffenseContext);
        m_pDefenseStateMachine->Update(delta, mechControllerComponent.DefenseContext);
    });
}

#pragma optimize("", on)

} // namespace WingsOfSteel
