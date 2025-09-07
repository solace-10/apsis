#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>
#include <glm/glm.hpp>

namespace WingsOfSteel::TheBrightestStar
{

enum class AIStrikecraftState
{
    APPROACH,
    ATTACK,
    BREAK,
    REPOSITION
};

class AIStrikecraftControllerComponent : public Pandora::IComponent
{
public:
    AIStrikecraftControllerComponent() 
        : m_State(AIStrikecraftState::APPROACH)
        , m_StateTimer(0.0f)
        , m_AttackTimer(0.0f)
        , m_BreakTimer(0.0f)
        , m_OptimalRange(500.0f)
        , m_MinRange(100.0f)
        , m_MaxRange(800.0f)
        , m_FiringAngle(15.0f)
        , m_AttackDuration(3.0f)
        , m_BreakDuration(2.0f)
        , m_LastTargetPosition(0.0f)
        , m_BreakDirection(1.0f, 0.0f, 0.0f)
    {}
    
    ~AIStrikecraftControllerComponent() {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["optimal_range"] = m_OptimalRange;
        json["min_range"] = m_MinRange;
        json["max_range"] = m_MaxRange;
        json["firing_angle"] = m_FiringAngle;
        json["attack_duration"] = m_AttackDuration;
        json["break_duration"] = m_BreakDuration;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        if (json.contains("optimal_range")) m_OptimalRange = json["optimal_range"];
        if (json.contains("min_range")) m_MinRange = json["min_range"];
        if (json.contains("max_range")) m_MaxRange = json["max_range"];
        if (json.contains("firing_angle")) m_FiringAngle = json["firing_angle"];
        if (json.contains("attack_duration")) m_AttackDuration = json["attack_duration"];
        if (json.contains("break_duration")) m_BreakDuration = json["break_duration"];
    }

    void SetTarget(Pandora::EntitySharedPtr pEntity) { m_pTarget = pEntity; }
    Pandora::EntitySharedPtr GetTarget() const { return m_pTarget.lock(); }

    AIStrikecraftState GetState() const { return m_State; }
    void SetState(AIStrikecraftState state) 
    { 
        m_State = state; 
        m_StateTimer = 0.0f;
    }

    void UpdateTimers(float deltaTime) 
    { 
        m_StateTimer += deltaTime;
        m_AttackTimer += deltaTime;
        m_BreakTimer += deltaTime;
    }

    bool ShouldFire(float distanceToTarget, float angleToTarget) const
    {
        return m_State == AIStrikecraftState::ATTACK &&
               distanceToTarget >= m_MinRange &&
               distanceToTarget <= m_MaxRange &&
               angleToTarget <= m_FiringAngle;
    }

    bool ShouldChangeState() const
    {
        switch (m_State)
        {
            case AIStrikecraftState::ATTACK:
                return m_StateTimer >= m_AttackDuration;
            case AIStrikecraftState::BREAK:
                return m_StateTimer >= m_BreakDuration;
            default:
                return false;
        }
    }

    float GetOptimalRange() const { return m_OptimalRange; }
    float GetMinRange() const { return m_MinRange; }
    float GetMaxRange() const { return m_MaxRange; }
    float GetStateTimer() const { return m_StateTimer; }
    
    const glm::vec3& GetLastTargetPosition() const { return m_LastTargetPosition; }
    void SetLastTargetPosition(const glm::vec3& pos) { m_LastTargetPosition = pos; }
    
    const glm::vec3& GetBreakDirection() const { return m_BreakDirection; }
    void SetBreakDirection(const glm::vec3& dir) { m_BreakDirection = dir; }

    const glm::vec3& GetRepositionTarget() const { return m_RepositionTarget; }
    void SetRepositionTarget(const glm::vec3& pos) { m_RepositionTarget = pos; }

private:
    Pandora::EntityWeakPtr m_pTarget;
    AIStrikecraftState m_State{ AIStrikecraftState::APPROACH };
    float m_StateTimer{ 0.0f };
    float m_AttackTimer{ 0.0f };
    float m_BreakTimer{ 0.0f };
    
    float m_OptimalRange{ 500.0f };
    float m_MinRange{ 100.0f };
    float m_MaxRange{ 800.0f };
    float m_FiringAngle{ 15.0f };
    float m_AttackDuration{ 3.0f };
    float m_BreakDuration{ 2.0f };
    
    glm::vec3 m_LastTargetPosition{ 0.0f };
    glm::vec3 m_BreakDirection{ 1.0f, 0.0f, 0.0f };
    glm::vec3 m_RepositionTarget{ 0.0f };
};

REGISTER_COMPONENT(AIStrikecraftControllerComponent, "ai_strikecraft_controller")

} // namespace WingsOfSteel::TheBrightestStar