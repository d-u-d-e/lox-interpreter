#include <environment.hpp>
#include <interpreter.hpp>

expr::value Environment::get(const Token &name) const
{
    auto it = values.find(name.get_lexeme());
    if (it != values.end())
    {
        return it->second;
    }

    if (enclosing != nullptr){
        return enclosing->get(name);
    }

    throw Interpreter::RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}

void Environment::define(const std::string &name, const expr::value &value)
{
    values[name] = value;
}

void Environment::assign(const Token &name, const expr::value &value)
{
    if (values.find(name.get_lexeme()) != values.end()){
        values[name.get_lexeme()] = value;
        return;
    }

    if (enclosing != nullptr){
        enclosing->assign(name, value);
        return;
    }

    throw Interpreter::RuntimeError(name, "Undefined variable '" + name.get_lexeme() + "'.");
}