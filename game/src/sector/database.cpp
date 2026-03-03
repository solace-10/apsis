#include <emscripten/fetch.h>
#include <sstream>

#include <core/log.hpp>

#include "sector/database.hpp"
#include "sector/sector.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

Database::Database()
{
    m_GetAllObjectsEndpoint = "https://nsorbisfunctionsnspcxszg-get-all-objects.functions.fnc.fr-par.scw.cloud";
}

void Database::GetAllObjects(OnAllObjectsReceivedCallback onAllObjectsReceivedCallback, OnDatabaseErrorCallback onDatabaseErrorCallback)
{
    m_Requests[m_RequestIndex] = DatabaseCallbacks{ onAllObjectsReceivedCallback, onDatabaseErrorCallback };
    
    Log::Info() << "Database request " << m_RequestIndex << ": Getting all objects at " << m_GetAllObjectsEndpoint;
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData = reinterpret_cast<void*>(m_RequestIndex);
    attr.onsuccess = [](emscripten_fetch_t* pFetch) {

        const uint32_t requestIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFetch->userData));
        Database* pDatabase = Game::Get()->GetSector()->GetDatabase();
        DatabaseCallbacks& callbacks = pDatabase->m_Requests[requestIndex];
        
        Log::Info() << "Database request " << requestIndex << ": downloaded " << pFetch->numBytes << " bytes.";

        Json::Data data = Json::Data::parse(pFetch->data, pFetch->data + pFetch->numBytes, nullptr, false);
        if (data.is_discarded())
        {
            callbacks.onDatabaseErrorCallback("Failed to parse JSON");
        }
        else
        {
            callbacks.onAllObjectsReceivedCallback(data);
        }
        
        emscripten_fetch_close(pFetch);
    };
    attr.onerror = [](emscripten_fetch_t* pFetch) {
        const uint32_t requestIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFetch->userData));
        Database* pDatabase = Game::Get()->GetSector()->GetDatabase();
        DatabaseCallbacks& callbacks = pDatabase->m_Requests[requestIndex];

        std::stringstream errorMessage;
        errorMessage << "HTTP " << pFetch->status << " (" << pFetch->statusText << ") for " << pFetch->url;

        Log::Error() << "Database request " << requestIndex << ": " << errorMessage.str();
        callbacks.onDatabaseErrorCallback(errorMessage.str());

        emscripten_fetch_close(pFetch);
    };

    m_RequestIndex++;
    emscripten_fetch(&attr, m_GetAllObjectsEndpoint.c_str());
}

} // namespace WingsOfSteel
