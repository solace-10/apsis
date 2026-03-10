#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include "sector/private/database_impl.hpp"

namespace WingsOfSteel::Private
{

class DatabaseWeb : public DatabaseImpl
{
public:
    DatabaseWeb(Database& parent);
    ~DatabaseWeb() override;

    void GetAllObjects() override;
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
