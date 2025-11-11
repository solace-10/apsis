#include <glm/gtx/rotate_vector.hpp>

#include "systems/shield_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/ghost_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <scene/systems/physics_simulation_system.hpp>


namespace WingsOfSteel
{

ShieldSystem::~ShieldSystem()
{
}

void ShieldSystem::Initialize(Scene* pScene)
{
}

void ShieldSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<ShieldComponent, GhostComponent, ModelComponent>();
    view.each([delta, this](const auto entity, ShieldComponent& shieldComponent, GhostComponent& ghostComponent, ModelComponent& modelComponent) {
        EntitySharedPtr pShieldEntity = shieldComponent.GetOwner().lock();
        if (!pShieldEntity)
        {
            return;
        }

        EntitySharedPtr pParentEntity = pShieldEntity->GetParent().lock();
        if (!pParentEntity)
        {
            return;
        }

        const glm::mat4& worldTransform = pParentEntity->GetComponent<TransformComponent>().transform;
        ghostComponent.SetWorldTransform(worldTransform);

        UpdateShieldState(delta, shieldComponent, modelComponent);
    });
}

void ShieldSystem::UpdateShieldState(float delta, ShieldComponent& shieldComponent, ModelComponent& modelComponent)
{
    float shieldPowerShaderValue = 0.0f;
    
    if (shieldComponent.CurrentState == ShieldState::Inactive && shieldComponent.WantedState == ShieldState::Active)
    {
        shieldComponent.CurrentState = ShieldState::PoweringUp;
        shieldComponent.ShieldPower = 0.0f;
    }
    else if (shieldComponent.CurrentState == ShieldState::PoweringUp && shieldComponent.WantedState == ShieldState::Active)
    {
        if (shieldComponent.PowerUpDuration <= 0.0f)
        {
            shieldComponent.CurrentState = ShieldState::Active;
            shieldPowerShaderValue = 1.0f;
        }
        else
        {
            shieldComponent.ShieldPower += delta * 1.0f / shieldComponent.PowerUpDuration;
            if (shieldComponent.ShieldPower >= 1.0f)
            {
                shieldComponent.CurrentState = ShieldState::Active;
                shieldPowerShaderValue = 1.0f;
            }
            else
            {
                shieldPowerShaderValue = shieldComponent.ShieldPower / shieldComponent.PowerUpDuration;
            }
        }
    }
    else if (shieldComponent.CurrentState == ShieldState::Active && shieldComponent.WantedState == ShieldState::Active)
    {
        shieldPowerShaderValue = 1.0f;
    }
    else if (shieldComponent.CurrentState == ShieldState::Active && shieldComponent.WantedState == ShieldState::Inactive)
    {
        shieldPowerShaderValue = 1.0f;
        shieldComponent.CurrentState = ShieldState::PoweringDown;
        shieldComponent.ShieldPower = shieldComponent.PowerDownDuration;
    }
    else if (shieldComponent.CurrentState == ShieldState::PoweringDown && shieldComponent.WantedState == ShieldState::Inactive)
    {
        if (shieldComponent.PowerDownDuration <= 0.0f)
        {
            shieldComponent.CurrentState = ShieldState::Inactive;
            shieldPowerShaderValue = 0.0f;
        }
        else
        {
            shieldComponent.ShieldPower -= delta * 1.0f / shieldComponent.PowerDownDuration;
            if (shieldComponent.ShieldPower <= 0.0f)
            {
                shieldComponent.CurrentState = ShieldState::Inactive;
                shieldPowerShaderValue = 0.0f;
            }
            else
            {
                shieldPowerShaderValue = shieldComponent.ShieldPower / shieldComponent.PowerDownDuration;
            }
        }
    }
    else if (shieldComponent.CurrentState == ShieldState::PoweringUp && shieldComponent.WantedState == ShieldState::Inactive)
    {
        shieldComponent.CurrentState = ShieldState::PoweringDown;
    }
    else if (shieldComponent.CurrentState == ShieldState::PoweringDown && shieldComponent.WantedState == ShieldState::Active)
    {
        shieldComponent.CurrentState = ShieldState::PoweringUp;
    }

    modelComponent.SetShaderParameter("shield_power", shieldPowerShaderValue);
}

} // namespace WingsOfSteel
