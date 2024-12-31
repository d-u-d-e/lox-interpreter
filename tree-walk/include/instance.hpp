#pragma once
#include <class.hpp>
#include <unordered_map>
#include <token.hpp>

class LoxInstance
{
public:
  LoxInstance(std::shared_ptr<LoxClass> klass) : klass(std::move(klass)) {}
  std::string to_string() const { return klass->to_string() + " instance"; }
  expr::Value get(const Token & name) const;
  void set(const Token & name, const expr::Value &value);

private:
  std::unordered_map<std::string, expr::Value> fields;
  std::shared_ptr<LoxClass> klass;
};