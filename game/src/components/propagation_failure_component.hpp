#pragma once

#include <chrono>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

#include "space/sgp4.hpp"

namespace WingsOfSteel
{

// The propagator's verdict that an object can no longer be placed, latched so that it sticks.
//
// It has to be latched because SGP4 reports its errors per step: an object whose perigee has
// dropped below one earth radius reports Decayed there and propagates cleanly half an orbit later,
// so deciding frame by frame would blink it in and out rather than take it off the screen.
//
// The error and the instant are kept rather than a bare flag because Decayed says the object has
// re-entered while MeanElementsOutOfRange only says the model gave up, and a catalogue view may
// want to tell them apart.
//
// Its presence is what retires an object - see SpaceObjectRenderSystem::RetireFailedSpaceObjects().
class PropagationFailureComponent : public IComponent
{
public:
    PropagationFailureComponent() = default;

    PropagationFailureComponent(SGP4Error error, const std::chrono::system_clock::time_point& instant)
        : m_Error(error)
        , m_Instant(instant)
    {
    }

    ~PropagationFailureComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    SGP4Error GetError() const { return m_Error; }
    const std::chrono::system_clock::time_point& GetInstant() const { return m_Instant; }

private:
    SGP4Error m_Error{ SGP4Error::None };
    std::chrono::system_clock::time_point m_Instant;
};

REGISTER_COMPONENT(PropagationFailureComponent, "propagation_failure")

} // namespace WingsOfSteel
