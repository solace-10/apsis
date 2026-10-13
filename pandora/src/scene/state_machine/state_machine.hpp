#pragma once

#include "state.hpp"

#include <array>
#include <memory>
#include <vector>

#include <magic_enum.hpp>

namespace WingsOfSteel
{

template <typename TStateEnum, typename TContext>
class StateMachine
{
public:
    using StatePtr = std::unique_ptr<State<TStateEnum, TContext>>;

    StateMachine() = default;
    ~StateMachine() = default;

    void AddState(TStateEnum id, StatePtr pState)
    {
        m_States[static_cast<size_t>(id)] = std::move(pState);
    }

    void AddTransition(TStateEnum from, TStateEnum to)
    {
        m_Transitions[static_cast<size_t>(from)].push_back(to);
    }

    void AddTransitions(TStateEnum from, std::initializer_list<TStateEnum> toStates)
    {
        for (const auto& to : toStates)
        {
            m_Transitions[static_cast<size_t>(from)].push_back(to);
        }
    }

    void SetInitialState(TStateEnum state)
    {
        m_InitialState = state;
    }

    // Returns true if the transition was successful, false if invalid.
    bool TransitionTo(TStateEnum state, TContext& context)
    {
        const TStateEnum currentState = context.currentState.value();
        const size_t currentIndex = static_cast<size_t>(currentState);
        const size_t targetIndex = static_cast<size_t>(state);

        // Check if this is a valid transition
        const auto& validTransitions = m_Transitions[currentIndex];
        if (!validTransitions.empty())
        {
            bool isValidTransition = false;
            for (const auto& validState : validTransitions)
            {
                if (validState == state)
                {
                    isValidTransition = true;
                    break;
                }
            }

            if (!isValidTransition)
            {
                return false;
            }
        }
        else if (currentState != state)
        {
            // No transitions defined from current state, and we're not transitioning to self
            return false;
        }

        // Check if target state exists
        if (m_States[targetIndex] == nullptr)
        {
            return false;
        }

        // Exit current state
        State<TStateEnum, TContext>* pCurrentState = m_States[currentIndex].get();
        if (pCurrentState != nullptr)
        {
            pCurrentState->OnExit(context);
        }

        // Transition to new state
        context.currentState = state;
        State<TStateEnum, TContext>* pNewState = m_States[targetIndex].get();

        // Enter new state
        if (pNewState != nullptr)
        {
            pNewState->OnEnter(context);
        }

        return true;
    }

    void Update(float delta, TContext& context)
    {
        if (!context.currentState)
        {
            context.currentState = GetInitialState();
            const size_t initialIndex = static_cast<size_t>(GetInitialState());
            State<TStateEnum, TContext>* pInitialState = m_States[initialIndex].get();
            if (pInitialState != nullptr)
            {
                pInitialState->OnEnter(context);
            }
        }
        
        const size_t currentIndex = static_cast<size_t>(context.currentState.value());
        State<TStateEnum, TContext>* pCurrentState = m_States[currentIndex].get();

        if (pCurrentState != nullptr)
        {
            std::optional<TStateEnum> requestedState = pCurrentState->Update(delta, context);
            if (requestedState.has_value())
            {
                TransitionTo(requestedState.value(), context);
            }
        }
    }

    TStateEnum GetCurrentState(const TContext& context) const
    {
        return context.currentState.value();
    }

    const char* GetCurrentStateName(const TContext& context) const
    {
        return magic_enum::enum_name(context.currentState.value()).data();
    }

    TStateEnum GetInitialState() const
    {
        return m_InitialState;
    }

private:
    static constexpr size_t StateCount = magic_enum::enum_count<TStateEnum>();

    std::array<StatePtr, StateCount> m_States;
    std::array<std::vector<TStateEnum>, StateCount> m_Transitions;
    TStateEnum m_InitialState{};
};

} // namespace WingsOfSteel
