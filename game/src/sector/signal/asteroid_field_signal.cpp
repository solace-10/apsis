#include <core/random.hpp>
#include <pandora.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>

#include "sector/phenomena/asteroid_field.hpp"
#include "sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

AsteroidFieldSignal::AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters)
: Signal(position, signalParameters)
{
    const nlohmann::json& contents = signalParameters["contents"];
    for (int i = 0; i < contents.size(); i++)
    {
        AddContent(contents[i]["item"].get<std::string>(), contents[i]["spawn_chance"].get<float>());
    }
}

AsteroidFieldSignal::~AsteroidFieldSignal()
{

}

Pandora::EntitySharedPtr AsteroidFieldSignal::Spawn(Pandora::Scene* pScene) const
{
    using namespace Pandora;
    EntitySharedPtr pAsteroidField = pScene->CreateEntity();

    DebugRenderComponent& debugRenderComponent = pAsteroidField->AddComponent<DebugRenderComponent>();
    debugRenderComponent.color = Color::SandyBrown;
    debugRenderComponent.shape = DebugRenderShape::Circle;
    debugRenderComponent.radius = 10.0f;

    TransformComponent& transformComponent = pAsteroidField->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), GetPosition());

    return pAsteroidField;
}

const AsteroidFieldSignalContents AsteroidFieldSignal::GetContents() const
{
    return m_Contents;
}

void AsteroidFieldSignal::AddContent(const std::string& itemName, float chance)
{
    m_Contents.push_back({itemName, chance});
}

} // namespace WingsOfSteel::TheBrightestStar