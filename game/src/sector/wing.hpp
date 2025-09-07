#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Wing);
class Wing
{
public:
    Wing() = default;
    ~Wing() = default;

    void Add(EntitySharedPtr pEntity);
    std::vector<EntitySharedPtr> Get() const;

private:
    std::vector<EntityWeakPtr> m_Entities;
};

inline void Wing::Add(EntitySharedPtr pEntity)
{
    m_Entities.push_back(pEntity);
}

inline std::vector<EntitySharedPtr> Wing::Get() const
{
    std::vector<EntitySharedPtr> entities;
    for (auto& pEntityWeak : m_Entities)
    {
        EntitySharedPtr pEntity = pEntityWeak.lock();
        if (pEntity)
        {
            entities.push_back(pEntity);
        }
    }
    return entities;
}

} // namespace WingsOfSteel