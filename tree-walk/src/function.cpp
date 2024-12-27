#include <function.hpp>
#include <interpreter.hpp>
#include <return.hpp>

expr::Value LoxFunction::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
    auto env = std::make_unique<Environment>(closure);
    auto &params = declaration->params;
    for (size_t i = 0; i < params.size(); i++)
    {
        env->define(params[i].get_lexeme(), args[i]);
    }

    try
    {
        interpreter.execute_block(declaration->body, std::move(env));
    }
    catch (Return &ret){
        return ret.value;
    }
    // function implicitely returns nil if no return is encountered
    return expr::Value();
}

int LoxFunction::arity() const
{
    return declaration->params.size();
}

std::string LoxFunction::to_string() const
{
    return "<fn " + declaration->name.get_lexeme() + ">";
}