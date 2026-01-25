#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

class SpaceObject;
DECLARE_SMART_PTR(Entity);

class WebInterop
{
public:
    WebInterop();
    ~WebInterop();

    static WebInterop* GetInstance();

    void NotifySpaceObjectSelected(const SpaceObject* pSpaceObject);
    void NotifySpaceObjectDeselected();
    void NotifySpaceObjectUpdated(const SpaceObject* pSpaceObject);

private:
    static WebInterop* s_pInstance;
};

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
