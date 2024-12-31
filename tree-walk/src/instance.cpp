#include <instance.hpp>
#include <interpreter.hpp>

expr::Value LoxInstance::get(const Token &name) const
{
  auto it = fields.find(name.get_lexeme());
  if(it != fields.end()) {
    return it->second;
  }

  auto method = klass->find_method(name.get_lexeme());

  if(method) {
    return expr::Value(method);
  }

  throw Interpreter::RuntimeError(name, "Undefined property '" + name.get_lexeme() + "'.");
}

void LoxInstance::set(const Token &name, const expr::Value &value)
{
  fields[name.get_lexeme()] = value;
}