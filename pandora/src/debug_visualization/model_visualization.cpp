#include "debug_visualization/model_visualization.hpp"

#include "pandora.hpp"
#include "render/debug_render.hpp"
#include "resources/resource_model.hpp"

namespace WingsOfSteel::Pandora
{

ModelVisualization::ModelVisualization()
{
}

ModelVisualization::~ModelVisualization()
{
}

void ModelVisualization::Render(ResourceModel* pModel, const glm::mat4& transform)
{
    // TODO: Implement model visualization update logic

    glm::vec4 translation = transform[3];
    glm::vec3 position(translation.x, translation.y, translation.z);

    // GetDebugRender()->Sphere(position, Color::Pink, 10.0f);

    // GetDebugRender()->Circle(position, glm::vec3(0.0f, 1.0f, 0.0f), Color::Pink, 10.0f, 20.0f);

    // GetDebugRender()->Label("Attachment", position, Color::Red, 0.5f);

    for (auto& attachment : pModel->GetAttachmentPoints())
    {
        glm::mat4 worldTransform = transform * attachment.m_ModelTransform;
        GetDebugRender()->AxisTriad(worldTransform, 1.0f, 10.0f);

        glm::vec3 labelPosition(worldTransform[3].x, worldTransform[3].y, worldTransform[3].z);
        GetDebugRender()->Label(attachment.m_Name, labelPosition, Color::White, 0.75f);
    }
}

void ModelVisualization::SetEnabled(Mode mode, bool state)
{
    if (state)
    {
        m_Mode |= static_cast<uint32_t>(mode);
    }
    else
    {
        m_Mode &= ~static_cast<uint32_t>(mode);
    }
}

bool ModelVisualization::IsEnabled(Mode mode) const
{
    return (m_Mode & static_cast<uint32_t>(mode)) != 0;
}

} // namespace WingsOfSteel::Pandora
