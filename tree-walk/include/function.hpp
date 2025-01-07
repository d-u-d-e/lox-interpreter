#pragma once
#include <callable.hpp>
#include <memory>

class Environment;
class LoxInstance;
namespace stmt
{
  class Function;
};

class LoxFunction : public LoxCallable
{
public:
  LoxFunction(std::shared_ptr<const stmt::Function> decl, std::shared_ptr<Environment> closure,
              bool is_initializer)
      : declaration(std::move(decl)), closure(std::move(closure)), is_initializer(is_initializer)
  {}
  expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override;
  int arity() const override;
  std::string to_string() const override;
  std::shared_ptr<LoxFunction> bind(const std::shared_ptr<LoxInstance> &instance);

private:
  std::shared_ptr<const stmt::Function> declaration;
  std::shared_ptr<Environment> closure;
  bool is_initializer;
};