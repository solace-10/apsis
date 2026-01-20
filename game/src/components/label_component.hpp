#pragma once

#include <string>

#include <glm/vec2.hpp>

#include <render/vertex_types.hpp>
#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class LabelComponent : public IComponent
{
public:
    LabelComponent() = default;
    
    LabelComponent(const std::string& text)
    : m_Text(text)
    {}
    
    ~LabelComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    void SetText(const std::string& text) { m_Text = text; m_VertexData.clear(); }
    const std::string& GetText() const { return m_Text; }
    void SetScreenSpacePosition(const glm::vec2& position) { m_ScreenSpacePosition = position; }
    const glm::vec2& GetScreenSpacePosition() const { return m_ScreenSpacePosition; }
    void SetVertexData(const std::vector<VertexP2C4UV>& vertexData) { m_VertexData = vertexData; }
    const std::vector<VertexP2C4UV>& GetVertexData() const { return m_VertexData; }
    void SetOccluded(bool occluded) { m_IsOccluded = occluded; }
    bool IsOccluded() const { return m_IsOccluded; }

private:
    std::string m_Text{ "UNKNOWN" };
    glm::vec2 m_ScreenSpacePosition{ 0.0f };
    std::vector<VertexP2C4UV> m_VertexData;
    bool m_IsOccluded{ true };
};

REGISTER_COMPONENT(LabelComponent, "label")

} // namespace WingsOfSteel
