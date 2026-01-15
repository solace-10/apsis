#include "space_objects/space_object_catalogue.hpp"

namespace WingsOfSteel
{

SpaceObjectCatalogue::SpaceObjectCatalogue()
{
}

SpaceObjectCatalogue::~SpaceObjectCatalogue()
{
}

void SpaceObjectCatalogue::Add(const SpaceObject& spaceObject)
{
    m_SpaceObjects[spaceObject.GetNoradCatalogueId()] = spaceObject;
}

const SpaceObject* SpaceObjectCatalogue::GetByNoradId(uint32_t noradId) const
{
    auto it = m_SpaceObjects.find(noradId);
    if (it != m_SpaceObjects.end())
    {
        return &it->second;
    }
    return nullptr;
}

} // namespace WingsOfSteel
