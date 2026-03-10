#pragma once

#if defined(TARGET_PLATFORM_NATIVE)

#include "sector/private/database_impl.hpp"

namespace WingsOfSteel::Private
{

class DatabaseNative : public DatabaseImpl
{
public:
    DatabaseNative(Database& parent);
    ~DatabaseNative() override;

    void GetAllObjects() override;
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_NATIVE
