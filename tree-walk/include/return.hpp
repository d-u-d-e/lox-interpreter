#pragma once
#include <exception>
#include <ast/expr.hpp>

struct Return : public std::exception
{
public:
    explicit Return(expr::Value &&value) : value(std::move(value)) {}
    Return() = default;
    expr::Value value{};
};