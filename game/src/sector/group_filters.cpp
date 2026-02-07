#include <core/log.hpp>

#include "sector/group_filters.hpp"

namespace WingsOfSteel
{

void GroupFilters::RegisterGroupFilter(const std::string& name, const std::string& displayName, const std::string& color, bool enabled)
{
    const uint8_t index = static_cast<uint8_t>(m_GroupFilters.size());
    GroupFilter groupFilter(name, displayName, color, enabled, index);
    m_GroupFilters.push_back(std::move(groupFilter));
    m_NameToGroupFilterIndex[name] = index;
    
    if (m_GroupFilters.size() >= GroupFilters::MaximumSupportedGroupFilters)
    {
        Log::Error() << "Number of group filters exceeds component capacity.";
    }
}

std::vector<std::string> GroupFilters::GetGroupFilterNames() const
{
    std::vector<std::string> names;

    for (const GroupFilter& groupFilter : m_GroupFilters)
    {
        names.push_back(groupFilter.GetName());
    }

    return names;  
}

GroupFilter* GroupFilters::GetGroupFilter(const std::string& name)
{
    const auto& it = m_NameToGroupFilterIndex.find(name);
    if (it == m_NameToGroupFilterIndex.cend())
    {
        return nullptr;
    }
    else
    {
        return &m_GroupFilters[it->second];
    }
}

GroupFilter* GroupFilters::GetGroupFilter(size_t index)
{
    if (index >= m_GroupFilters.size())
    {
        return nullptr;
    }
    else
    {
        return &m_GroupFilters[index];
    }
}

GroupFilters::Mask GroupFilters::GetCurrentMask() const
{
    Mask mask;
    for (const GroupFilter& groupFilter : m_GroupFilters)
    {
        if (groupFilter.IsEnabled())
        {
            mask.set(groupFilter.GetBitIndex());
        }
    }
    return mask;
}

} // namespace WingsOfSteel
