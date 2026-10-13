#pragma once

namespace WingsOfSteel
{
class InputSystem;
}

namespace WingsOfSteel::Private
{

class InputSystemImpl
{
public:
    InputSystemImpl(InputSystem& parent) : m_Parent(parent) {}
    virtual ~InputSystemImpl() {}

    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual bool IsCursorLocked() const = 0;

protected:
    InputSystem& m_Parent;
};

} // namespace WingsOfSteel::Private
