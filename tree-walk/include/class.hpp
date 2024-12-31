#pragma once
#include <callable.hpp>
#include <memory>

class Environment;
namespace stmt
{
  class Class;
};

class LoxClass : public LoxCallable
{
public:
  LoxClass(const std::string &name) : name(name) {}
  std::string to_string() const { return name; }
  int arity() const override { return 0; }
  expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override;

private:
  std::string name;
};