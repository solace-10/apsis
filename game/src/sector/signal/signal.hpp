#pragma once

#include <glm/vec3.hpp>

#include <nlohmann/json.hpp>
#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

// Base Signal class. Cannot be instantiated directly.
DECLARE_SMART_PTR(Signal);
class Signal
{
public:
    virtual ~Signal();

    const glm::vec3& GetPosition() const;
    float GetSignalDifficulty() const;

protected:
    Signal(const glm::vec3& position);

    void SetSignalDifficulty(float difficulty);

private:
    glm::vec3 m_Position;
    float m_Difficulty;
};

} // namespace WingsOfSteel::TheBrightestStar