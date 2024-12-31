#include <class.hpp>
#include <interpreter.hpp>
#include <instance.hpp>

expr::Value LoxClass::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
    return std::make_shared<LoxInstance>(shared_from_this());
}