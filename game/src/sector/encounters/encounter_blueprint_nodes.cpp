#include <imgui/text_editor/text_editor.hpp>

#include <pandora.hpp>
#include <core/log.hpp>

#include "sector/encounters/encounter_window.hpp"
#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "sector/encounters/encounter.hpp"

namespace WingsOfSteel::TheBrightestStar
{

NodeSharedPtr BlueprintNodeFactory::CreateNode(const std::string& nodeName)
{
    if (nodeName == "Sector entered")
    {
        return std::make_shared<SectorEnteredNode>();
    }
    else if (nodeName == "Exit sector")
    {
        return std::make_shared<SectorExitNode>();
    }
    else if (nodeName == "Encounter stage")
    {
        return std::make_shared<EncounterStageNode>();
    }
    else if (nodeName == "Encounter option")
    {
        return std::make_shared<EncounterOptionNode>();
    }
    else if (nodeName == "Dice")
    {
        return std::make_shared<DiceNode>();
    }
    else if (nodeName == "Image")
    {
        return std::make_shared<ImageNode>();
    }
    else if (nodeName == "String")
    {
        return std::make_shared<StringNode>();
    }
    else if (nodeName == "Outcome conditional")
    {
        return std::make_shared<OutcomeConditionalNode>();
    }

    Pandora::Log::Warning() << "Unknown node name: " << nodeName;
    return nullptr;
}

////////////////////////////////////////////////////////////
// Sector entered
////////////////////////////////////////////////////////////

SectorEnteredNode::SectorEnteredNode()
: Node("Sector entered", ImColor(255, 0, 0))
{
    Outputs.emplace_back("", PinType::Flow);
}

NodeType SectorEnteredNode::GetNodeType() const
{
    return NodeType::SectorEntered;
}

Node::ExecutionResult SectorEnteredNode::Execute(Encounter* pEncounter, float delta)
{
    Node* pLinkedNode = pEncounter->GetFirstLinkedNode(&Outputs[0], true);
    if (pLinkedNode)
    {
        SetNextNode(pLinkedNode);
    }

    return ExecutionResult::Complete;
}

////////////////////////////////////////////////////////////
// Sector exit
////////////////////////////////////////////////////////////

SectorExitNode::SectorExitNode()
: Node("Exit sector", ImColor(255, 0, 0))
{
    Inputs.emplace_back("", PinType::Flow);
}

NodeType SectorExitNode::GetNodeType() const
{
    return NodeType::SectorExit;
}

////////////////////////////////////////////////////////////
// Encounter stage
////////////////////////////////////////////////////////////

EncounterStageNode::EncounterStageNode()
: Node("Encounter stage", ImColor(5, 250, 191))
{
    Inputs.emplace_back("", PinType::Flow);
    Inputs.emplace_back("Description", PinType::String);
    Inputs.emplace_back("Image", PinType::Image);
    Inputs.emplace_back("Option 1", PinType::EncounterOption);
    Inputs.emplace_back("Option 2", PinType::EncounterOption);
    Inputs.emplace_back("Option 3", PinType::EncounterOption);
    Outputs.emplace_back("Selected 1", PinType::Flow);
    Outputs.emplace_back("Selected 2", PinType::Flow);
    Outputs.emplace_back("Selected 3", PinType::Flow);
    Outputs.emplace_back("Outcome", PinType::Outcome);
}

NodeType EncounterStageNode::GetNodeType() const
{
    return NodeType::EncounterStage;
}

void EncounterStageNode::OnExecutionStarted(Encounter* pEncounter)
{
    auto pEncounterWindow = pEncounter->GetEncounterWindow();
    if (pEncounterWindow)
    {
        auto pLinkedNode = pEncounter->GetFirstLinkedNode(&Inputs[1]);
        if (pLinkedNode && pLinkedNode->GetNodeType() == NodeType::String)
        {
            auto pStringNode = static_cast<StringNode*>(pLinkedNode);
            pEncounterWindow->AppendText(pStringNode->Value);
        }

        pEncounterWindow->SetCurrentStage(std::static_pointer_cast<EncounterStageNode>(shared_from_this()));
    }

    m_SelectedOption.reset();
}

void EncounterStageNode::OnOptionSelected(Option option)
{
    m_SelectedOption = option;
}

Node::ExecutionResult EncounterStageNode::Execute(Encounter* pEncounter, float delta)
{
    if (m_SelectedOption)
    {
        const uint32_t optionIndex = static_cast<uint32_t>(m_SelectedOption.value());

        auto pLinkedNode = pEncounter->GetFirstLinkedNode(&Outputs[optionIndex], true);
        if (pLinkedNode)
        {
            SetNextNode(pLinkedNode);
        }
        else
        {
            Pandora::Log::Warning() << "No linked node found for option: " << optionIndex;
        }
        return ExecutionResult::Complete;
    }

    return Node::ExecutionResult::Continue;
}

////////////////////////////////////////////////////////////
// Encounter option
////////////////////////////////////////////////////////////

EncounterOptionNode::EncounterOptionNode()
: Node("Encounter option", ImColor(5, 250, 191))
{
    Inputs.emplace_back("Dice", PinType::Dice);
    Inputs.emplace_back("Description", PinType::String);
    Outputs.emplace_back("Option", PinType::EncounterOption);
}

NodeType EncounterOptionNode::GetNodeType() const
{
    return NodeType::EncounterOption;
}

////////////////////////////////////////////////////////////
// String
////////////////////////////////////////////////////////////

StringNode::StringNode()
: Node("String", ImColor(124, 21, 153), NodeDisplayType::String)
{
    Outputs.emplace_back("Value", PinType::String);
}

NodeType StringNode::GetNodeType() const
{
    return NodeType::String;
}

nlohmann::json StringNode::Serialize() const
{
    nlohmann::json data = Node::Serialize();
    data["value"] = Value;
    return data;
}

void StringNode::Deserialize(const nlohmann::json& data)
{
    Node::Deserialize(data);
    auto valueIt = data.find("value");
    if (valueIt != data.end() && valueIt->is_string())
    {
        Value = valueIt->get<std::string>();
    }
}

////////////////////////////////////////////////////////////
// Dice
////////////////////////////////////////////////////////////

DiceNode::DiceNode()
: Node("Dice", ImColor(255, 165, 0), NodeDisplayType::Dice)
{
    Outputs.emplace_back("Value", PinType::Dice);
}

NodeType DiceNode::GetNodeType() const
{
    return NodeType::Dice;
}

nlohmann::json DiceNode::Serialize() const
{
    nlohmann::json data = Node::Serialize();
    data["value"] = magic_enum::enum_name(Value);
    return data;
}

void DiceNode::Deserialize(const nlohmann::json& data)
{
    Node::Deserialize(data);
    
    if (data.contains("value"))
    {
        Value = magic_enum::enum_cast<DiceCategory>(data["value"].get<std::string>()).value_or(DiceCategory::Electronics);
    }
}

////////////////////////////////////////////////////////////
// Image
////////////////////////////////////////////////////////////

ImageNode::ImageNode()
: Node("Image", ImColor(165, 255, 0))
{
    Outputs.emplace_back("Value", PinType::Image);
}

NodeType ImageNode::GetNodeType() const
{
    return NodeType::Image;
}

////////////////////////////////////////////////////////////
// OutcomeConditional
////////////////////////////////////////////////////////////

OutcomeConditionalNode::OutcomeConditionalNode()
: Node("Outcome conditional", ImColor(255, 80, 0))
{
    Inputs.emplace_back("", PinType::Flow);
    Inputs.emplace_back("Outcome", PinType::Outcome);
    Outputs.emplace_back("Positive", PinType::Flow);
    Outputs.emplace_back("Neutral", PinType::Flow);
    Outputs.emplace_back("Negative", PinType::Flow);
}

NodeType OutcomeConditionalNode::GetNodeType() const
{
    return NodeType::OutcomeConditional;
}

Node::ExecutionResult OutcomeConditionalNode::Execute(Encounter* pEncounter, float delta)
{
    EncounterOutcome outcome = EncounterOutcome::Neutral;
    auto pInputOutcomeNode = pEncounter->GetFirstLinkedNode(&Inputs[1]);
    if (pInputOutcomeNode)
    {
        IHasOutcomeOutput* pOutcomeInterface = dynamic_cast<IHasOutcomeOutput*>(pInputOutcomeNode);
        if (pOutcomeInterface)
        {
            outcome = pOutcomeInterface->GetEncounterOutcome();
        }
    }

    const uint32_t optionIndex = static_cast<uint32_t>(outcome);
    assert(optionIndex < Outputs.size());

    auto pLinkedNode = pEncounter->GetFirstLinkedNode(&Outputs[optionIndex], true);
    if (pLinkedNode)
    {
        SetNextNode(pLinkedNode);
    }
    else
    {
        Pandora::Log::Warning() << "No linked node found for option: " << optionIndex;
    }
    return ExecutionResult::Complete;
}


} // namespace WingsOfSteel::TheBrightestStar
