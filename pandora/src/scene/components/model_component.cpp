#include "resources/resource_system.hpp"
#include "pandora.hpp"

#include "scene/components/model_component.hpp"

namespace WingsOfSteel::Pandora
{

ModelComponent::ModelComponent(const std::string& path)
{
    GetResourceSystem()->RequestResource(path, [this](ResourceSharedPtr pResource) {
        this->m_pResource = std::dynamic_pointer_cast<ResourceModel>(pResource);
    });
}

} // namespace WingsOfSteel::Pandora