#include <function.hpp>
#include <instance.hpp>
#include <interpreter.hpp>
#include <return.hpp>

expr::Value LoxFunction::call(Interpreter &interpreter, const std::vector<expr::Value> &args)
{
  auto env = std::make_unique<Environment>(closure);
  auto &params = declaration->params;
  for(size_t i = 0; i < params.size(); i++) {
    env->define(params[i].get_lexeme(), args[i]);
  }

  try {
    interpreter.execute_block(declaration->body, std::move(env));
  }
  catch(Return &ret) {
    /* When explicitely calling init(), a return statement inside that method should return the
    `this` instance, not nil; when not explicitely calling init(), i.e. using the class name, this
    is already handled in LoxClass::call */

    if(is_initializer) {
      return closure->get_at(0, "this");
    }

    return ret.value;
  }

  // the contructor returns the instance if no return statement is encountered
  if(is_initializer) {
    return closure->get_at(0, "this");
  }

  // function implicitely returns nil if no return is encountered
  return expr::Value();
}

int LoxFunction::arity() const { return declaration->params.size(); }

std::string LoxFunction::to_string() const { return "<fn " + declaration->name.get_lexeme() + ">"; }

std::shared_ptr<LoxFunction> LoxFunction::bind(const std::shared_ptr<LoxInstance> &instance)
{
  auto env = std::make_shared<Environment>(closure);
  env->define("this", expr::Value(instance));
  return std::make_shared<LoxFunction>(declaration, env, is_initializer);
}