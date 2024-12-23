#pragma once
#include <unordered_map>
#include <ast/expr.hpp>
#include <token.hpp>

class Environment
{
public:
    Environment() {}

    void define(const std::string &name, const expr::value &value);
    void assign(const Token &name, const expr::value &value);
    expr::value get(const Token &name) const;

private:
    std::unordered_map<std::string, expr::value> values;
};