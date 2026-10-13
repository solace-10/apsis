
#include <cstdint>

#include <glm/vec3.hpp>

#include "imgui/idebugui.hpp"
#include "render/rendersystem.hpp"
#include "scene/entity.hpp"

namespace WingsOfSteel
{

enum class Tonemapper : uint32_t
{
    Neutral,
    ACES,
    AgX
};

struct AmbientLight
{
    glm::vec3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity{ 1.0f };
};

struct DirectionalLight
{
    float Angle{ 0.0f };
    float Pitch{ 0.0f };
    glm::vec3 Direction{ 0.0f, 1.0f, 0.0f };
    glm::vec3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity{ 1.0f };
};

class LightingSystem : public IDebugUI
{
public:
    LightingSystem();
    ~LightingSystem();

    void DrawDebugUI() override;

    void Update();

    const AmbientLight& GetAmbientLight() const;
    const DirectionalLight& GetDirectionalLight() const;

    // In stops: the scene is scaled by 2^exposure before it is tonemapped.
    float GetExposure() const;

    // How much of the blurred scene is mixed into it; 0 turns bloom off.
    float GetBloomStrength() const;

    Tonemapper GetTonemapper() const;

private:
    void DrawDebugGizmo();

    AmbientLight m_AmbientLight;
    EntityWeakPtr m_pAmbientLightEntity;

    DirectionalLight m_DirectionalLight;
    EntityWeakPtr m_pDirectionalLightEntity;

    float m_Exposure{ 1.0f };
    float m_BloomStrength{ 0.04f };
    Tonemapper m_Tonemapper{ Tonemapper::ACES };
};

} // namespace WingsOfSteel
