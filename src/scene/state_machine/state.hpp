#pragma once

#include <optional>

namespace WingsOfSteel
{

template <typename TStateEnum, typename TContext>
class State
{
public:
    virtual ~State() = default;

    virtual void OnEnter(TContext& context) {}
    virtual std::optional<TStateEnum> Update(float delta, TContext& context) { return std::nullopt; }
    virtual void OnExit(TContext& context) {}
    virtual TStateEnum GetStateID() const = 0;
};

} // namespace WingsOfSteel
