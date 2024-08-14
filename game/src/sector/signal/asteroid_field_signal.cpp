#include <core/random.hpp>

#include "sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

AsteroidFieldSignal::AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters)
: Signal(position)
{
    SetSignalDifficulty(signalParameters["signal_difficulty"].get<float>());

    const nlohmann::json& contents = signalParameters["contents"];
    for (int i = 0; i < contents.size(); i++)
    {
        AddContent(contents[i]["item"].get<std::string>(), contents[i]["spawn_chance"].get<float>());
    }
}

AsteroidFieldSignal::~AsteroidFieldSignal()
{

}

const AsteroidFieldSignalContents AsteroidFieldSignal::GetContents() const
{
    return m_Contents;
}

void AsteroidFieldSignal::AddContent(const std::string& itemName, float chance)
{
    m_Contents.push_back({itemName, chance});
}

} // namespace WingsOfSteel::TheBrightestStar