#pragma once

#include <optional>

namespace WingsOfSteel::Pandora
{

template <typename T, typename V = void>
class Result;

// Specialization for value type
template <typename T, typename V>
class Result
{
public:
    // Construct with error
    Result(T error)
        : error_(error)
        , value_(std::nullopt)
    {
    }
    // Construct with value
    Result(V value)
        : error_()
        , value_(std::move(value))
    {
    }
    // Construct with error and value (rare, but possible)
    Result(T error, V value)
        : error_(error)
        , value_(std::move(value))
    {
    }

    bool has_value() const { return value_.has_value(); }
    explicit operator bool() const { return has_value(); }
    const V& value() const { return value_.value(); }
    V& value() { return value_.value(); }
    const T& error() const { return error_; }
    T& error() { return error_; }

private:
    T error_;
    std::optional<V> value_;
};

// Specialization for void value type (error-only)
template <typename T>
class Result<T, void>
{
public:
    Result(T error)
        : error_(error)
    {
    }

    bool has_value() const { return false; }
    explicit operator bool() const { return false; }
    const T& error() const { return error_; }
    T& error() { return error_; }

private:
    T error_;
};

} // namespace WingsOfSteel::Pandora