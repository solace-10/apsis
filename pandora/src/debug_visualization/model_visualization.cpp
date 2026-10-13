#include "debug_visualization/model_visualization.hpp"

#include "pandora.hpp"
#include "render/debug_render.hpp"
#include "resources/resource_model.hpp"

namespace WingsOfSteel
{

ModelVisualization::ModelVisualization()
{
}

ModelVisualization::~ModelVisualization()
{
}

void ModelVisualization::Render(ResourceModel* pModel, const glm::mat4& transform)
{
    if (IsEnabled(Mode::AttachmentPoints))
    {
        for (auto& attachment : pModel->GetAttachmentPoints())
        {
            const glm::mat4 worldTransform = transform * attachment.m_ModelTransform;
            GetDebugRender()->AxisTriad(worldTransform, 1.0f, 10.0f);

            const glm::vec3 labelPosition(worldTransform[3]);
            GetDebugRender()->Label(attachment.m_Name, labelPosition, Color::White, 0.75f);
        }
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

} // namespace WingsOfSteel
