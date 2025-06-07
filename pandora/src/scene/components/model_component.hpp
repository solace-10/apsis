#pragma once

#include <string>

#include "resources/resource_model.hpp"

namespace WingsOfSteel::Pandora
{

class ModelComponent
{
public:
    ModelComponent(ResourceModelSharedPtr pModel)
        : m_pResource(pModel)
    {
    }
    ~ModelComponent() {}

    ResourceModel* GetModel() const;

private:
    ResourceModelSharedPtr m_pResource;
};

inline ResourceModel* ModelComponent::GetModel() const
{
    return m_pResource.get();
}

} // namespace WingsOfSteel::Pandora