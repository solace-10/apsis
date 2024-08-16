#include <core/random.hpp>

#include "sector/phenomena/asteroid_field.hpp"
#include "sector/signal/asteroid_field_signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

AsteroidFieldSignal::AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters)
: Signal(position, signalParameters)
{
    const nlohmann::json& contents = signalParameters["contents"];
    for (int i = 0; i < contents.size(); i++)
    {
        AddContent(contents[i]["item"].get<std::string>(), contents[i]["spawn_chance"].get<float>());
    }
}

AsteroidFieldSignal::~AsteroidFieldSignal()
{

}

Pandora::EntitySharedPtr AsteroidFieldSignal::Spawn() const
{
    return std::make_shared<AsteroidField>(this);
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