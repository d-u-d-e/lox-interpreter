#pragma once
#include <callable.hpp>
#include <memory>
#include <unordered_map>

class Environment;
class LoxFunction;

class LoxClass : public LoxCallable, public std::enable_shared_from_this<LoxClass>
{
public:
  LoxClass(const std::string &name, std::shared_ptr<const LoxClass> &&superclass,
           std::unordered_map<std::string, std::shared_ptr<LoxFunction>> &&methods)
      : name(name), superclass(std::move(superclass)), methods(std::move(methods))
  {}
  std::string to_string() const { return name; }
  int arity() const override;
  expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override;
  std::shared_ptr<LoxFunction> find_method(const std::string &name) const;

private:
  std::string name;
  std::shared_ptr<const LoxClass> superclass;
  std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods;
};