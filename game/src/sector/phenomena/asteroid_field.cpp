#include <vector>

#include <render/debug_render.hpp>
#include <pandora.hpp>

#include "sector/phenomena/asteroid_field.hpp"
#include "sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

AsteroidField::AsteroidField(const AsteroidFieldSignal* pSignal)
: m_pSignal(pSignal)
{

}

AsteroidField::~AsteroidField()
{

}

void AsteroidField::OnAddedToScene(Pandora::Scene* pScene)
{
    Pandora::Entity::OnAddedToScene(pScene);
}

void AsteroidField::Update(float delta) 
{
    using namespace Pandora;

    Entity::Update(delta);

    GetDebugRender()->Circle(
        m_pSignal->GetPosition(),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Pandora::Color::Green,
        20.0f,
        20.0f
    );

    GetDebugRender()->Label(m_pSignal->GetName(), m_pSignal->GetPosition(), Color::White);
}

void AsteroidField::Render(wgpu::RenderPassEncoder renderPass)
{
    Pandora::Entity::Render(renderPass);
}

} // namespace WingsOfSteel::TheBrightestStar