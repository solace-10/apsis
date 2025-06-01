#pragma once

#include <functional>
#include <map>

namespace WingsOfSteel::Pandora
{

// A signal object may call multiple slots with the
// same signature. You can connect functions to the signal
// which will be called when the emit() method on the
// signal object is invoked. Any argument passed to emit()
// will be passed to the given functions.

using SignalId = uint32_t;

template <typename... Args>
class Signal
{

public:
    Signal() = default;
    ~Signal() = default;

    // Copy constructor and assignment create a new signal.
    Signal(Signal const& /*unused*/) {}

    Signal& operator=(Signal const& other)
    {
        if (this != &other)
        {
            DisconnectAll();
        }
        return *this;
    }

    // Move constructor and assignment operator work as expected.
    Signal(Signal&& other) noexcept
        : m_Slots(std::move(other.m_Slots))
        , m_CurrentId(other.m_CurrentId)
    {
    }

    Signal& operator=(Signal&& other) noexcept
    {
        if (this != &other)
        {
            m_Slots = std::move(other.m_Slots);
            m_CurrentId = other.m_CurrentId;
        }

        return *this;
    }

    // Connects a std::function to the signal. The returned
    // value can be used to disconnect the function again.
    SignalId Connect(std::function<void(Args...)> const& slot) const
    {
        m_Slots.insert(std::make_pair(++m_CurrentId, slot));
        return m_CurrentId;
    }

    // Convenience method to connect a member function of an
    // object to this Signal.
    template <typename T>
    SignalId ConnectMember(T* inst, void (T::*func)(Args...))
    {
        return Connect([=](Args... args) {
            (inst->*func)(args...);
        });
    }

    // Convenience method to connect a const member function
    // of an object to this Signal.
    template <typename T>
    SignalId ConnectMember(T* inst, void (T::*func)(Args...) const)
    {
        return Connect([=](Args... args) {
            (inst->*func)(args...);
        });
    }

    // Disconnects a previously connected function.
    void Disconnect(SignalId id) const
    {
        m_Slots.erase(id);
    }

    // Disconnects all previously connected functions.
    void DisconnectAll() const
    {
        m_Slots.clear();
    }

    // Calls all connected functions.
    void Emit(Args... p)
    {
        for (auto const& it : m_Slots)
        {
            it.second(p...);
        }
    }

private:
    mutable std::map<SignalId, std::function<void(Args...)>> m_Slots;
    mutable SignalId m_CurrentId{ 0 };
};

} // namespace WingsOfSteel::Pandora
