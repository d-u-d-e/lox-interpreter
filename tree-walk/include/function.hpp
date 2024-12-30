#pragma once
#include <callable.hpp>
#include <memory>

class Environment;
namespace stmt
{
  class Function;
};

class LoxFunction : public LoxCallable
{
public:
  LoxFunction(std::shared_ptr<const stmt::Function> decl, std::shared_ptr<Environment> closure)
      : declaration(std::move(decl)), closure(std::move(closure))
  {}
  expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override;
  int arity() const override;
  std::string to_string() const override;

private:
  std::shared_ptr<const stmt::Function> declaration;
  std::shared_ptr<Environment> closure;
};