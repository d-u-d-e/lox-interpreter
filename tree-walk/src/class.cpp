#include <class.hpp>
#include <instance.hpp>
#include <function.hpp>
#include <interpreter.hpp>

expr::Value LoxClass::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
  auto instance = std::make_shared<LoxInstance>(shared_from_this());
  auto ctor = find_method("init");
  if(ctor != nullptr) {
    ctor->bind(instance)->call(interpreter, args);
  }
  return instance;
}

std::shared_ptr<LoxFunction> LoxClass::find_method(const std::string &name) const
{
  if(methods.find(name) != methods.end()) {
    return methods.at(name);
  }

  // else look in the superclass
  if(superclass != nullptr) {
    return superclass->find_method(name);
  }

  return nullptr;
}

int LoxClass::arity() const
{
  auto ctor = find_method("init");
  if(ctor != nullptr) {
    return ctor->arity();
  }
  return 0;
}