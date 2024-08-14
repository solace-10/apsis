#include "sector/signal/signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Signal::Signal(const glm::vec3& position)
: m_Position(position)
, m_Difficulty(0.0f)
{
}

Signal::~Signal()
{

}

const glm::vec3& Signal::GetPosition() const
{
    return m_Position;
}

float Signal::GetSignalDifficulty() const
{
    return m_Difficulty;
}

void Signal::SetSignalDifficulty(float difficulty)
{
    m_Difficulty = difficulty;
}

} // namespace WingsOfSteel::TheBrightestStar