#pragma once

#include <unordered_map>

#include "core/smart_ptr.hpp"
#include "space_objects/space_object.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(SpaceObjectCatalogue);
class SpaceObjectCatalogue
{
public:
    SpaceObjectCatalogue();
    ~SpaceObjectCatalogue();

    void Add(const SpaceObject& spaceObject);
    const SpaceObject* GetByNoradId(uint32_t noradId) const;
    size_t GetCount() const { return m_SpaceObjects.size(); }

private:
    std::unordered_map<uint32_t, SpaceObject> m_SpaceObjects;
};

}
