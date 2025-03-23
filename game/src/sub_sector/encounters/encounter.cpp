#include "encounter.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Encounter::Encounter(Pandora::ResourceDataStoreSharedPtr pDataStore)
: m_pDataStore(pDataStore)
{
    Load();
}

Encounter::~Encounter()
{
}

void Encounter::Update()
{
}

void Encounter::Save()
{
    nlohmann::json data;
    data["nodes"] = nlohmann::json::array();
    for (auto& pNode : m_Nodes)
    {
        data["nodes"].push_back(pNode->Serialize());
    }
    
    data["links"] = nlohmann::json::array();
    m_pDataStore->Inject(data);
}

void Encounter::Revert()
{
    m_Nodes.clear();
    Load();
}

void Encounter::Load()
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
