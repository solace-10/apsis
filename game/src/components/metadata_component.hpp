#pragma once

#include <bitset>
#include <string>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

#include "sector/group_filter.hpp"
#include "sector/group_filters.hpp"

namespace WingsOfSteel
{

class MetadataComponent : public IComponent
{
public:
    MetadataComponent() = default;
    ~MetadataComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        m_ObjectName = Json::DeserializeString(pContext, json, "name");
        m_ObjectId = Json::DeserializeString(pContext, json, "id");
        m_NoradCatalogueId = Json::DeserializeUnsignedInteger(pContext, json, "norad_id");
    }

    bool m_IsImportant{ false };

    const std::string& GetObjectName() const { return m_ObjectName; }
    const std::string& GetObjectId() const { return m_ObjectId; }
    uint32_t GetNoradCatalogueId() const { return m_NoradCatalogueId; }

    void AddToGroupFilter(GroupFilter* pGroupFilter);
    bool IsInGroupFilter(GroupFilter* pGroupFilter) const;
    GroupFilters::Mask GetGroupFilterMask() const { return m_GroupFilters; }
    void SetVisible(bool state) { m_Visible = state; }
    bool IsVisible() const { return m_Visible; }

private:
    std::string m_ObjectName{ "UNKNOWN" };
    std::string m_ObjectId{ "0" };
    uint32_t m_NoradCatalogueId{ 0 };

    // We keep a bitset of all the group filters the parent entity is part of.
    GroupFilters::Mask m_GroupFilters;
    bool m_Visible{ false };
};

inline void MetadataComponent::AddToGroupFilter(GroupFilter* pGroupFilter)
{
    m_GroupFilters[pGroupFilter->GetBitIndex()] = true;
}

inline bool MetadataComponent::IsInGroupFilter(GroupFilter* pGroupFilter) const
{
    return m_GroupFilters[pGroupFilter->GetBitIndex()];
}

REGISTER_COMPONENT(MetadataComponent, "metadata")

} // namespace WingsOfSteel
