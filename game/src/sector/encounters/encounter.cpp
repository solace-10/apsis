#include <core/log.hpp>
#include <resources/resource_data_store.hpp>
#include <pandora.hpp>

#include "encounter.hpp"
#include "encounter_blueprint_nodes.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Encounter::Encounter(const std::string& name, Pandora::ResourceDataStoreSharedPtr pDataStore)
: m_pDataStore(pDataStore)
, m_Name(name)
{
    Load();
}

Encounter::~Encounter()
{
}

void Encounter::Start()
{
    assert(!m_Started);
    m_Started = false;

    for (auto& pNode : m_Nodes)
    {
        if (pNode->GetNodeType() == NodeType::SectorEntered)
        {
            m_Started = true;
            m_pCurrentNode = pNode.get();
            m_pCurrentNode->OnExecutionStarted(this);
            break;
        }
    }
}

void Encounter::Update(float delta)
{
    if (m_pCurrentNode == nullptr || !m_Started)
    {
        return;
    }

    for (int numExecutions = 0; numExecutions < 10; ++numExecutions)
    {
        if (m_pCurrentNode == nullptr)
        {
            break;
        }

        Node::ExecutionResult result = m_pCurrentNode->Execute(this, delta);
        if (result == Node::ExecutionResult::Complete)
        {
            m_pCurrentNode->OnExecutionEnded(this);
            m_pCurrentNode = m_pCurrentNode->GetNextNode();
            if (m_pCurrentNode)
            {
                m_pCurrentNode->OnExecutionStarted(this);
            }
        }
        else if (result == Node::ExecutionResult::Continue)
        {
            break;
        }
    }
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
    for (auto& pLink : m_Links)
    {
        nlohmann::json link
        {
            {"id", pLink->ID.Get()},
            {"start", pLink->StartPinID.Get()},
            {"end", pLink->EndPinID.Get()}
        };
        data["links"].push_back(link);
    }
    m_pDataStore->Inject(data);
}

void Encounter::Revert()
{
    m_Nodes.clear();
    Load();
}

void Encounter::Load()
{
    const nlohmann::json& encounterData = m_pDataStore->Data();
    const auto nodesIt = encounterData.find("nodes");
    if (nodesIt != encounterData.end() && nodesIt->is_array())
    {
        for (const auto& nodeData : *nodesIt)
        {
            const auto nodeNameIt = nodeData.find("name");
            if (nodeNameIt != nodeData.end() && nodeNameIt->is_string())
            {
                const std::string nodeName = nodeNameIt->get<std::string>();
                NodeUniquePtr pNode = BlueprintNodeFactory::CreateNode(nodeName);
                if (pNode)
                {
                    pNode->Deserialize(nodeData);
                    AddNode(std::move(pNode));
                }
                else
                {
                    Pandora::Log::Warning() << "Unknown node type: " << nodeName;
                }
            }
        }
    }

    const auto linksIt = encounterData.find("links");
    if (linksIt != encounterData.end() && linksIt->is_array())
    {
        for (const auto& linkData : *linksIt)
        {
            const auto linkIdIt = linkData.find("id");
            const auto linkStartIt = linkData.find("start");
            const auto linkEndIt = linkData.find("end");
            if (linkIdIt != linkData.end() && linkIdIt->is_number_unsigned() &&
                linkStartIt != linkData.end() && linkStartIt->is_number_unsigned() &&
                linkEndIt != linkData.end() && linkEndIt->is_number_unsigned())
            {
                LinkUniquePtr pLink = std::make_unique<Link>
                (
                    linkIdIt->get<BlueprintId>(),
                    linkStartIt->get<BlueprintId>(),
                    linkEndIt->get<BlueprintId>()
                );
                AddLink(std::move(pLink)); 
            }
        }
    }
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

void Encounter::AddNode(NodeUniquePtr pNode)
{
    assert(!m_Started);
    using namespace Pandora;
    if (pNode->ID.Get() == InvalidBlueprintId)
    {
        Log::Warning() << "Unable to add encounter node '" << pNode->Name << "' has it has an invalid ID.";
        return;
    }

    std::unordered_map<BlueprintId, Pin*> pinsToRegister;

    for (auto& pin : pNode->Inputs)
    {
        if (pin.ID.Get() == InvalidBlueprintId)
        {
            Log::Warning() << "Unable to add input pin '" << pin.Name << "' to encounter node '" << pNode->Name << "' has it has an invalid ID.";
            return;
        }

        pinsToRegister[pin.ID.Get()] = &pin;
    }

    for (auto& pin : pNode->Outputs)
    {
        if (pin.ID.Get() == InvalidBlueprintId)
        {
            Log::Warning() << "Unable to add output pin '" << pin.Name << "' to encounter node '" << pNode->Name << "' has it has an invalid ID.";
            return;
        }

        pinsToRegister[pin.ID.Get()] = &pin;
    }

    // Only add the pins to m_PinIdToPinMap if all the pins for this node are valid.
    for (auto& pinToRegister : pinsToRegister)
    {
        m_PinIdToPinMap[pinToRegister.first] = pinToRegister.second;
    }

    m_Nodes.emplace_back(std::move(pNode));
}

bool Encounter::RemoveNode(BlueprintId id)
{
    assert(!m_Started);
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [id](const NodeUniquePtr& node) { return node->ID.Get() == id; });
    if (it != m_Nodes.end())
    {
        auto clearPinFn = [this](Pin& pin)
        {
            m_PinIdToPinMap.erase(pin.ID.Get());

            auto it = std::find_if(m_Links.begin(), m_Links.end(), [pin](const LinkUniquePtr& link) { return link->StartPinID == pin.ID || link->EndPinID == pin.ID; });
            if (it != m_Links.end())
            {
                RemoveLink((*it)->ID.Get());
            }
        };

        for (auto& pin : (*it)->Inputs)
        {
            clearPinFn(pin);
        }

        for (auto& pin : (*it)->Outputs)
        {
            clearPinFn(pin);
        }
        
        m_Nodes.erase(it);
        return true;
    }
    return false;
}

const std::vector<Link*> Encounter::GetLinks() const
{
    std::vector<Link*> links;
    for (const auto& link : m_Links)
    {
        links.push_back(link.get());
    }
    return links;
}

const std::vector<Node*> Encounter::GetLinkedNodes(Pin* pPin) const
{
    std::vector<Node*> nodes;
    for (const auto& pLink : m_Links)
    {
        Node* pNode = GetLinkedNode(pPin, pLink.get());
        if (pNode)
        {
            nodes.push_back(pNode);
        }
    }
    return nodes;
}

Node* Encounter::GetFirstLinkedNode(Pin* pPin) const
{
    for (const auto& pLink : m_Links)
    {
        Node* pNode = GetLinkedNode(pPin, pLink.get());
        if (pNode)
        {
            return pNode;
        }
    }
    return nullptr;
}

Node* Encounter::GetLinkedNode(Pin* pPin, Link* pLink) const
{
    Pin* pLinkedPin = nullptr;
    if (pLink->StartPinID.Get() == pPin->ID.Get())
    {
        pLinkedPin = GetPin(pLink->EndPinID.Get());
    }
    else if (pLink->EndPinID.Get() == pPin->ID.Get())
    {
        pLinkedPin = GetPin(pLink->StartPinID.Get());
    }

    if (pLinkedPin == nullptr)
    {
        return nullptr;
    }
    else
    {
        return pLinkedPin->Node;
    }
}

void Encounter::AddLink(LinkUniquePtr pLink)
{
    Pin* pStartPin = GetPin(pLink->StartPinID.Get());
    Pin* pEndPin = GetPin(pLink->EndPinID.Get());

    if (pStartPin && pEndPin)
    {
        pStartPin->Connected = true;
        pEndPin->Connected = true;
        m_Links.emplace_back(std::move(pLink));
    }
}

bool Encounter::RemoveLink(BlueprintId id)
{
    assert(!m_Started);
    auto it = std::find_if(m_Links.begin(), m_Links.end(), [id](const LinkUniquePtr& link) { return link->ID.Get() == id; });
    if (it != m_Links.end())
    {
        Link* pLink = it->get();
        Pin* pStartPin = GetPin(pLink->StartPinID.Get());
        if (pStartPin)
        {
            pStartPin->Connected = false;
        }

        Pin* pEndPin = GetPin(pLink->EndPinID.Get());
        if (pEndPin)
        {
            pEndPin->Connected = false;
        }
        
        m_Links.erase(it);
        return true;
    }
    return false;
}

Pin* Encounter::GetPin(BlueprintId id) const
{
    auto it = m_PinIdToPinMap.find(id);
    if (it != m_PinIdToPinMap.end())
    {
        return it->second;
    }
    return nullptr;
}

Node* Encounter::GetNode(BlueprintId id) const
{
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [id](const NodeUniquePtr& node) { return node->ID.Get() == id; });
    if (it != m_Nodes.end())
    {
        return it->get();
    }
    return nullptr;
}

} // namespace WingsOfSteel::TheBrightestStar
