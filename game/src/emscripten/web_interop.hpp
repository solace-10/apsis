#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);

class WebInterop
{
public:
    WebInterop();
    ~WebInterop();

    static WebInterop* GetInstance();

    void NotifySpaceObjectSelected(EntitySharedPtr pEntity);
    void NotifySpaceObjectDeselected();
    void NotifySpaceObjectUpdated(EntitySharedPtr pEntity);

private:
    static WebInterop* s_pInstance;
};

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
