#include <core/log.hpp>

#if defined(TARGET_PLATFORM_NATIVE)
#include "sector/private/native/database_native.hpp"
#elif defined(TARGET_PLATFORM_WEB)
#include "sector/private/web/database_web.hpp"
#endif

#include "sector/database.hpp"

namespace WingsOfSteel
{

Database::Database()
{
    m_GetAllObjectsEndpoint = "https://nsorbisfunctionsnspcxszg-get-all-objects.functions.fnc.fr-par.scw.cloud";

#if defined(TARGET_PLATFORM_NATIVE)
    m_pImpl = std::make_unique<Private::DatabaseNative>(*this);
#elif defined(TARGET_PLATFORM_WEB)
    m_pImpl = std::make_unique<Private::DatabaseWeb>(*this);
#endif
}

Database::~Database()
{
}

void Database::GetAllObjects(OnAllObjectsReceivedCallback onAllObjectsReceivedCallback, OnDatabaseErrorCallback onDatabaseErrorCallback)
{
    m_Requests[m_RequestIndex] = DatabaseCallbacks{ onAllObjectsReceivedCallback, onDatabaseErrorCallback };
    m_pImpl->GetAllObjects();
}

} // namespace WingsOfSteel
