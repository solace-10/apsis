#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "sector/group_filter.hpp"

namespace WingsOfSteel
{

class GroupFilters
{
public:
    GroupFilters() = default;
    ~GroupFilters() = default;

    void RegisterGroupFilter(const std::string& name, bool enabled);
    std::vector<std::string> GetGroupFilterNames() const;
    GroupFilter* GetGroupFilter(const std::string& name);
    GroupFilter* GetGroupFilter(size_t index);

private:
    std::vector<GroupFilter> m_GroupFilters;
    std::unordered_map<std::string, size_t> m_NameToGroupFilterIndex;
};
  
} // namespace WingsOfSteel
