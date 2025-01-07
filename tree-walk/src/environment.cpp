#include <environment.hpp>
#include <interpreter.hpp>

expr::Value Environment::get(const Token &name) const
{
  auto it = values.find(name.get_lexeme());
  if(it != values.end()) {
    return it->second;
  }

  if(enclosing != nullptr) {
    return enclosing->get(name);
  }

  throw Interpreter::RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

expr::Value Environment::get_at(int distance, const std::string &name) const
{
  return ancestor(distance)->values.at(name);
}

const Environment *Environment::ancestor(int distance) const
{
  auto env = this;
  for(int i = 0; i < distance; i++) {
    // assert enclosing is not null!
    env = env->enclosing.get();
  }
  return env;
}

Environment *Environment::ancestor(int distance)
{
  return const_cast<Environment *>(const_cast<const Environment *>(this)->ancestor(distance));
}

void Environment::define(const std::string &name, const expr::Value &value)
{
  values[name] = value;
}

void Environment::define(const std::string &name, expr::Value &&value)
{
  values.insert_or_assign(name, std::move(value));
}

void Environment::assign(const Token &name, const expr::Value &value)
{
  if(values.find(name.get_lexeme()) != values.end()) {
    values[name.get_lexeme()] = value;
    return;
  }

  if(enclosing != nullptr) {
    enclosing->assign(name, value);
    return;
  }

  throw Interpreter::RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::assign_at(int distance, const std::string &name, const expr::Value &value)
{
  ancestor(distance)->values[name] = value;
}