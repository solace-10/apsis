#pragma once

#include <vector>

#include "sector/signal/signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(AsteroidFieldSignal);
class AsteroidFieldSignal : public Signal
{
public:
    AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters);
    ~AsteroidFieldSignal() override;

    void AddContent(const std::string& itemName, float chance);

private:
    std::vector<std::pair<std::string, float>> m_Contents;
};

} // namespace WingsOfSteel::TheBrightestStar