#include <class.hpp>
#include <instance.hpp>
#include <interpreter.hpp>

expr::Value LoxClass::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
  return std::make_shared<LoxInstance>(shared_from_this());
}

std::shared_ptr<LoxFunction> LoxClass::find_method(const std::string &name) const
{
  if(methods.find(name) != methods.end()) {
    return methods.at(name);
  }
  return nullptr;
}