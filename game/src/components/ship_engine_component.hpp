#pragma once

namespace WingsOfSteel::TheBrightestStar
{

class ShipEngineComponent
{
public:
    float linearForce{ 0.0f };
    float torque{ 0.0f };
    float maximumLinearVelocity{ 0.0f };
    float maximumAngularVelocity{ 0.0f };
};

} // namespace WingsOfSteel::TheBrightestStar