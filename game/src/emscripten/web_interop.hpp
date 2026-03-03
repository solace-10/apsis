#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <string>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);
class GroupFilters;

class WebInterop
{
public:
    WebInterop();
    ~WebInterop();

    static WebInterop* GetInstance();

    void NotifySpaceObjectSelected(EntitySharedPtr pEntity);
    void NotifySpaceObjectDeselected();
    void NotifySpaceObjectUpdated(EntitySharedPtr pEntity);
    void NotifyGroupFiltersChanged(GroupFilters* pGroupFilters);

    static void SetGroupFilterEnabled(const std::string& groupId, bool enabled);

private:
    static WebInterop* s_pInstance;
};

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
