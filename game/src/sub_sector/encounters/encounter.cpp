#include "encounter.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Encounter::Encounter(Pandora::ResourceDataStoreSharedPtr pDataStore)
: m_pDataStore(pDataStore)
{
}

Encounter::~Encounter()
{
}

void Encounter::Update()
{
}

const std::vector<Node*> Encounter::GetNodes() const
{
    std::vector<Node*> nodes;
    for (const auto& node : m_Nodes)
    {
        nodes.push_back(node.get());
    }
    return nodes;
}

} // namespace WingsOfSteel::TheBrightestStar
