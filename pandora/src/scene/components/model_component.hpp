#pragma once

#include <string>

#include "resources/resource_model.hpp"
#include "resources/resource_system.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

class ModelComponent : public IComponent
{
public:
    ModelComponent() = default;
    ~ModelComponent() {}

    ResourceModelSharedPtr GetModel() const { return m_pResource; }
    
    void SetModel(ResourceModelSharedPtr pModel) 
    {
        m_pResource = pModel;
        m_ResourcePath = pModel->GetPath();
    }
    
    void SetModel(const std::string& resourcePath)
    {
        m_ResourcePath = resourcePath;
        GetResourceSystem()->RequestResource(m_ResourcePath, [this](ResourceSharedPtr pResource) {
            m_pResource = std::dynamic_pointer_cast<ResourceModel>(pResource);
        });
    }


    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["resource"] = m_ResourcePath;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        SetModel(DeserializeRequired<std::string>(json, "resource"));
    }

private:
    ResourceModelSharedPtr m_pResource;
    std::string m_ResourcePath;
};

REGISTER_COMPONENT(ModelComponent, "model")

} // namespace WingsOfSteel::Pandora