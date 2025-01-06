#pragma once
#include <ast/expr.hpp>
#include <token.hpp>
#include <unordered_map>

class Environment
{
public:
  Environment() {}
  Environment(std::shared_ptr<Environment> enclosing) : enclosing(std::move(enclosing)) {}

  void define(const std::string &name, const expr::Value &value);
  void define(const std::string &name, expr::Value &&value);
  void assign(const Token &name, const expr::Value &value);
  void assign_at(int distance, const std::string &name, const expr::Value &value);
  expr::Value get(const Token &name) const;
  expr::Value get_at(int distance, const std::string &name) const;
  std::shared_ptr<Environment> enclosing{};

private:
  const Environment *ancestor(int distance) const;
  Environment *ancestor(int distance);
  std::unordered_map<std::string, expr::Value> values;
};