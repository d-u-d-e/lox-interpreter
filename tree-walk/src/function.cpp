#include <function.hpp>
#include <interpreter.hpp>

expr::Value LoxFunction::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
    auto env = std::make_unique<Environment>(interpreter.get_env());
    auto &params = declaration->params;
    for (size_t i = 0; i < params.size(); i++)
    {
        env->define(params[i].get_lexeme(), args[i]);
    }

    interpreter.execute_block(declaration->body, std::move(env));
    return {};
}

int LoxFunction::arity() const
{
    return declaration->params.size();
}

std::string LoxFunction::to_string() const
{
    return "<fn " + declaration->name.get_lexeme() + ">";
}