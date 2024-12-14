#pragma once

#include <vector>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorSignal);

DECLARE_SMART_PTR(Hyperscape);
class Hyperscape
{
public:
    Hyperscape();
    ~Hyperscape();

    void Initialize();

    std::vector<SectorSignal*> GetSignals() const;

private:
    // Prototype: currently we have a single signal.
    SectorSignalUniquePtr m_pSignal;
};

} // namespace WingsOfSteel::TheBrightestStar