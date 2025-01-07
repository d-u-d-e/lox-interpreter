#pragma once
#include <class.hpp>
#include <token.hpp>

class LoxInstance : public std::enable_shared_from_this<LoxInstance>
{
public:
  LoxInstance(std::shared_ptr<LoxClass> klass) : klass(std::move(klass)) {}
  std::string to_string() const { return klass->to_string() + " instance"; }
  expr::Value get(const Token &name);
  void set(const Token &name, const expr::Value &value);

private:
  std::unordered_map<std::string, expr::Value> fields;
  std::shared_ptr<LoxClass> klass;
};