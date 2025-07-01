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

    ResourceModelSharedPtr GetModel() const { return m_pResource; }

private:
    ResourceModelSharedPtr m_pResource;
};

} // namespace WingsOfSteel::Pandora