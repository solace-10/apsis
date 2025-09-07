#pragma once

namespace WingsOfSteel
{

// Found in https://gamedev.net/forums/topic/329868-damped-spring-effects-for-camera/3149147/?page=1
// Use a damped spring to move v0 towards target given a current velocity, time over which the spring would
// cover 90% of the distance from rest and the delta time.
template<typename T>
void DampSpring(T& v0, const T& target, T& velocity, float time90, float delta)
{
    const float c0 = delta * 3.75f / time90;
    if (c0 >= 1.0f)
    {
        // If our distance to the target is too small, we go the whole way to prevent oscillation.
        v0 = target;
        velocity = T(0);
        return;
    }

    const T diff = target - v0;
    const T force = diff - 2.0f * velocity;
    v0 += velocity * c0;
    velocity += force * c0;
}

} // namespace WingsOfSteel