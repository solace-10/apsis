#pragma once

#include <string>

#include <glm/vec3.hpp>

#include <nlohmann/json.hpp>
#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel::TheBrightestStar
{

// Base Signal class. Cannot be instantiated directly.
DECLARE_SMART_PTR(Signal);
class Signal
{
public:
    virtual ~Signal();

    virtual Pandora::EntitySharedPtr Spawn(Pandora::Scene* pScene) const = 0;

    const std::string& GetName() const;
    const glm::vec3& GetPosition() const;
    float GetSignalDifficulty() const;

protected:
    Signal(const glm::vec3& position, const nlohmann::json& signalParameters);

private:
    std::string m_Name;
    glm::vec3 m_Position;
    float m_Difficulty;
};

} // namespace WingsOfSteel::TheBrightestStar