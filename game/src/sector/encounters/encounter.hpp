#pragma once

#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "encounter_blueprint_types.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
class Encounter
{
public:
    Encounter(Pandora::ResourceDataStoreSharedPtr pDataStore);
    ~Encounter();

    void Start();
    void Update(float delta);

    void Save();
    void Revert();

    void AddNode(NodeUniquePtr pNode);
    bool RemoveNode(BlueprintId id);
    const std::vector<Node*> GetNodes() const;

    void AddLink(LinkUniquePtr pLink);
    bool RemoveLink(BlueprintId id);
    const std::vector<Link*> GetLinks() const;
    const std::vector<Node*> GetLinkedNodes(Pin* pPin) const;
    Pin* GetPin(BlueprintId id) const;

protected:
    Node* GetNode(BlueprintId id) const;

private:
    void Load();

    Pandora::ResourceDataStoreSharedPtr m_pDataStore;
    std::vector<NodeUniquePtr> m_Nodes;
    std::vector<LinkUniquePtr> m_Links;
    std::unordered_map<BlueprintId, Pin*> m_PinIdToPinMap;
    Node* m_pCurrentNode{nullptr};
    bool m_Started{false};
};

} // namespace WingsOfSteel::TheBrightestStar