#include <instance.hpp>
#include <interpreter.hpp>

expr::Value LoxInstance::get(const Token &name) const
{
  auto it = fields.find(name.get_lexeme());
  if(it != fields.end()) {
    return it->second;
  }
  throw Interpreter::RuntimeError(name, "Undefined property '" + name.get_lexeme() + "'.");
}