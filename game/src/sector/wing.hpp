#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Wing);
class Wing
{
public:
    Wing() = default;
    ~Wing() = default;

    void Add(Pandora::EntitySharedPtr pEntity);
    std::vector<Pandora::EntitySharedPtr> Get() const;

private:
    std::vector<Pandora::EntityWeakPtr> m_Entities;
};

inline void Wing::Add(Pandora::EntitySharedPtr pEntity)
{
    m_Entities.push_back(pEntity);
}

inline std::vector<Pandora::EntitySharedPtr> Wing::Get() const
{
    std::vector<Pandora::EntitySharedPtr> entities;
    for (auto& pEntityWeak : m_Entities)
    {
        Pandora::EntitySharedPtr pEntity = pEntityWeak.lock();
        if (pEntity)
        {
            entities.push_back(pEntity);
        }
    }
    return entities;
}

} // namespace WingsOfSteel::TheBrightestStar