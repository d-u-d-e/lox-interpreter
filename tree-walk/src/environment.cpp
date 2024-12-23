#include <environment.hpp>
#include <interpreter.hpp>

expr::value Environment::get(const Token &name) const
{
    auto it = values.find(name.get_lexeme());
    if (it != values.end())
    {
        return it->second;
    }
    throw Interpreter::RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::define(const std::string &name, const expr::value &value)
{
    values[name] = value;
}