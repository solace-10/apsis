#include <core/log.hpp>
#include <core/random.hpp>
#include <pandora.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/tag_component.hpp>
#include <scene/components/transform_component.hpp>

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

    const float asteroidFieldRadius = Pandora::Random::Get(1000.0f, 1500.0f);
    const glm::vec3 asteroidFieldCenter = GetPosition();

    DebugRenderComponent& debugRenderComponent = pAsteroidField->AddComponent<DebugRenderComponent>();
    debugRenderComponent.color = Color::SandyBrown;
    debugRenderComponent.shape = DebugRenderShape::Circle;
    debugRenderComponent.radius = asteroidFieldRadius;

    TransformComponent& transformComponent = pAsteroidField->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), GetPosition());

    pAsteroidField->AddComponent<TagComponent>(GetName());

    int numAsteroids = static_cast<int>(asteroidFieldRadius / 10.0f);
    struct AsteroidCollisionData
    {
        glm::vec3 position;
        float radius;
    };
    std::vector<AsteroidCollisionData> data;
    data.reserve(numAsteroids);

    auto checkOverlap = [&data](const AsteroidCollisionData& toTest) -> bool
    {
        for (auto& asteroid : data)
        {
            const float x1 = toTest.position.x;
            const float x2 = asteroid.position.x;
            const float z1 = toTest.position.z;
            const float z2 = asteroid.position.z;
            const float distanceSquared = (x2 - x1) * (x2 - x1) + (z2 - z1) * (z2 - z1);
            const float radiusSum = toTest.radius + asteroid.radius;
            const float radiusSumSquared = radiusSum * radiusSum;
            if (distanceSquared <= radiusSumSquared)
            {
                return true;
            }
        }
        return false;
    };

    for (int i = 0; i < numAsteroids; i++)
    {
        for (int retries = 0; retries < 3; retries++)
        {
            AsteroidCollisionData asteroidCollisionData;
            asteroidCollisionData.position = asteroidFieldCenter + glm::vec3(Random::Get(-asteroidFieldRadius, asteroidFieldRadius), Random::Get(-100.0f, 100.0f), Random::Get(-asteroidFieldRadius, asteroidFieldRadius));
            asteroidCollisionData.radius = Random::Get(10.0f, 50.0f);

            if (glm::distance(asteroidFieldCenter, asteroidCollisionData.position) > asteroidFieldRadius || checkOverlap(asteroidCollisionData))
            {
                continue;
            }

            EntitySharedPtr pAsteroid = pScene->CreateEntity();
            DebugRenderComponent& debugRenderComponent = pAsteroid->AddComponent<DebugRenderComponent>();
            debugRenderComponent.color = Color::SandyBrown;
            debugRenderComponent.shape = DebugRenderShape::Circle;
            debugRenderComponent.radius = asteroidCollisionData.radius;
            TransformComponent& transformComponent = pAsteroid->AddComponent<TransformComponent>();
            transformComponent.transform = glm::translate(glm::mat4(1.0f), asteroidCollisionData.position);
            data.push_back(asteroidCollisionData);
            break;
        }
    }

    Pandora::Log::Info() << "Generated asteroid field with " << data.size() << " asteroids.";
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

std::string_view AsteroidFieldSignal::GenerateAsteroidContents() const
{
    const float roll = Pandora::Random::Get(1.0f);
    float accum = 0.0f;
    for (auto& pair : m_Contents)
    {
        accum += pair.second;
        if (roll <= accum)
        {
            return std::string_view{pair.first};
        }
    }
    
    return std::string_view();
}

} // namespace WingsOfSteel::TheBrightestStar