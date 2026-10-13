#pragma once

#include <memory>

#include <glm/mat4x4.hpp>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ResourceModel);

DECLARE_SMART_PTR(ModelVisualization);
class ModelVisualization
{
public:
    ModelVisualization();
    ~ModelVisualization();

    void Render(ResourceModel* pModel, const glm::mat4& transform);

    // clang-format off
    enum class Mode
    {
        None                = 0,
        AttachmentPoints    = 1 << 0,
    };
    // clang-format on

    void SetEnabled(Mode mode, bool state);
    bool IsEnabled(Mode mode) const;

private:
    uint32_t m_Mode{ 0 };
};

} // namespace WingsOfSteel