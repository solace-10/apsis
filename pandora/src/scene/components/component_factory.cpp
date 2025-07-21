#include "component_factory.hpp"

namespace WingsOfSteel::Pandora
{
    std::unordered_map<std::string, ComponentFactory::CreateFunc>* ComponentFactory::s_creators = nullptr;
} // namespace WingsOfSteel::Pandora