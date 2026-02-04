#pragma once

#include <string>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class MetadataComponent : public IComponent
{
public:
    MetadataComponent() = default;
    ~MetadataComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    std::string m_ObjectName{ "UNKNOWN" };
    std::string m_ObjectId{ "0" };
    uint32_t m_NoradCatalogueId{ 0 };
    bool m_IsImportant{ false };
};

REGISTER_COMPONENT(MetadataComponent, "metadata")

} // namespace WingsOfSteel
