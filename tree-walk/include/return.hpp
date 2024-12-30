#pragma once
#include <ast/expr.hpp>
#include <exception>

struct Return : public std::exception {
public:
  explicit Return(expr::Value &&value) : value(std::move(value)) {}
  Return() = default;
  expr::Value value{};
};