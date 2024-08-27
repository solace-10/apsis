#include "items/item_info.hpp"
#include "items/item.hpp"

namespace WingsOfSteel::TheBrightestStar
{

ItemSharedPtr ItemInfo::Create(int quantity /* = 1 */) const
{
    return std::make_shared<Item>(this, quantity);
}

} // namespace WingsOfSteel::TheBrightestStar