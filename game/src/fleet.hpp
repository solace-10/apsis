#pragma once

#include <core/smart_ptr.hpp>
#include <scene/scene.hpp>

#include <vector>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Fleet);
class Fleet
{
public:
    Fleet() {}
    ~Fleet() {}

    void AddShip(Pandora::EntitySharedPtr pShip);
    
    std::vector<Pandora::EntitySharedPtr> GetShips() const;

private:
    std::vector<Pandora::EntitySharedPtr> m_Ships;
};

inline void Fleet::AddShip(Pandora::EntitySharedPtr pShip)
{
    m_Ships.push_back(pShip);
}

inline std::vector<Pandora::EntitySharedPtr> Fleet::GetShips() const
{
    return m_Ships;
}

} // namespace WingsOfSteel::TheBrightestStar