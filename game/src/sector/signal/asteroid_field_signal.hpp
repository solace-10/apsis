#pragma once

#include <vector>

#include "sector/signal/signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

using AsteroidFieldSignalContents = std::vector<std::pair<std::string, float>>;

DECLARE_SMART_PTR(AsteroidFieldSignal);
class AsteroidFieldSignal : public Signal
{
public:
    AsteroidFieldSignal(const glm::vec3& position, const nlohmann::json& signalParameters);
    ~AsteroidFieldSignal() override;

    const AsteroidFieldSignalContents GetContents() const;

private:
    void AddContent(const std::string& itemName, float chance);

    AsteroidFieldSignalContents m_Contents;
};

} // namespace WingsOfSteel::TheBrightestStar