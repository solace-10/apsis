#pragma once

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(ItemInfo)

DECLARE_SMART_PTR(Item);
class Item
{
public:
    Item(const ItemInfo* pItemInfo, int quantity) 
    : m_pItemInfo(pItemInfo)
    , m_Quantity(quantity)
    {}

    ~Item() {}

    const ItemInfo* GetItemInfo() const;
    int GetQuantity() const;

private:
    const ItemInfo* m_pItemInfo;
    int m_Quantity;
};

inline const ItemInfo* Item::GetItemInfo() const
{
    return m_pItemInfo;
}

inline int Item::GetQuantity() const
{
    return m_Quantity;
}

} // namespace WingsOfSteel::TheBrightestStar