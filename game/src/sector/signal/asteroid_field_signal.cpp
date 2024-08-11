#include "sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

AsteroidFieldSignal::AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters)
: Signal(position, signalParameters)
{

}

AsteroidFieldSignal::~AsteroidFieldSignal()
{

}

void AsteroidFieldSignal::AddContent(const std::string& itemName, float chance)
{
    m_Contents.push_back({itemName, chance});
}

} // namespace WingsOfSteel::TheBrightestStar