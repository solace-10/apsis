#if defined(TARGET_PLATFORM_NATIVE)

#include <core/log.hpp>

#include "sector/private/native/database_native.hpp"
#include "sector/database.hpp"

namespace WingsOfSteel::Private
{

DatabaseNative::DatabaseNative(Database& parent)
    : DatabaseImpl(parent)
{
}

DatabaseNative::~DatabaseNative()
{
}

void DatabaseNative::GetAllObjects()
{
    Log::Warning() << "Database::GetAllObjects() is not implemented for native builds.";
}

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_NATIVE
