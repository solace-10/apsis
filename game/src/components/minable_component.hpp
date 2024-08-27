#pragma once

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Item);

class MinableComponent
{
public:
    MinableComponent(ItemSharedPtr pItem, int quantity) {}
    ~MinableComponent() {}

    Item* GetItem() const;
    int GetQuantity() const;
    int Mine();

private:
    ItemSharedPtr m_pItem;
    int m_Quantity;
};

} // namespace WingsOfSteel::TheBrightestStar