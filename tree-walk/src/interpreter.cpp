#include <chrono>
#include <class.hpp>
#include <interpreter.hpp>
#include <lox.hpp>
#include <memory>
#include <return.hpp>

expr::Value Interpreter::visit_binary_expr(const expr::Binary &expr)
{
  expr::Value left = evaluate(*expr.left);
  expr::Value right = evaluate(*expr.right);

  switch(expr.op.get_type()) {
  case Token::TokenType::MINUS:
    check_number_operands(expr.op, left, right);

    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) - std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) - std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) - std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) - std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::STAR:
    check_number_operands(expr.op, left, right);

    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) * std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) * std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) * std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) * std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::SLASH:
    check_number_operands(expr.op, left, right);

    if(left.is_int() && right.is_int()) {
      return (double)std::get<long long>(left.v) / std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) / std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) / std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) / std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::PLUS:

    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) + std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) + std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) + std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) + std::get<long long>(right.v);
    }
    else if(left.is_string() && right.is_string()) {
      return std::get<std::string>(left.v) + std::get<std::string>(right.v);
    }

    throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

  case Token::TokenType::GREATER:
    check_number_operands(expr.op, left, right);

    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) > std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) > std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) > std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) > std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::GREATER_EQUAL:
    check_number_operands(expr.op, left, right);

    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) >= std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) >= std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) >= std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) >= std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::LESS:
    check_number_operands(expr.op, left, right);
    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) < std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) < std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) < std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) < std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::LESS_EQUAL:
    check_number_operands(expr.op, left, right);
    if(left.is_int() && right.is_int()) {
      return std::get<long long>(left.v) <= std::get<long long>(right.v);
    }
    else if(left.is_double() && right.is_double()) {
      return std::get<double>(left.v) <= std::get<double>(right.v);
    }
    else if(left.is_int() && right.is_double()) {
      return std::get<long long>(left.v) <= std::get<double>(right.v);
    }
    else if(left.is_double() && right.is_int()) {
      return std::get<double>(left.v) <= std::get<long long>(right.v);
    }
    break;

  case Token::TokenType::EQUAL_EQUAL: return is_equal(left, right);

  case Token::TokenType::BANG_EQUAL: return !is_equal(left, right);
  }

  // unreachable
  throw std::runtime_error("interpreter error");
}
expr::Value Interpreter::visit_grouping_expr(const expr::Grouping &expr)
{
  return evaluate(*expr.expr);
}
expr::Value Interpreter::visit_literal_expr(const expr::Literal &expr) { return expr.value; }

bool Interpreter::is_truthy(const expr::Value &value)
{
  if(value.is_nil()) {
    return false;
  }
  else if(value.is_bool()) {
    return std::get<bool>(value.v);
  }
  return true;
}

bool Interpreter::is_equal(const expr::Value &left, const expr::Value &right)
{
  if(left.is_nil() && right.is_nil()) {
    return true;
  }
  else if(left.is_nil()) {
    return false;
  }

  if(left.is_double() && right.is_double()) {
    return std::get<double>(left.v) == std::get<double>(right.v);
  }
  else if(left.is_string() && right.is_string()) {
    return std::get<std::string>(left.v) == std::get<std::string>(right.v);
  }
  else if(left.is_bool() && right.is_bool()) {
    return std::get<bool>(left.v) == std::get<bool>(right.v);
  }
  else if(left.is_int() && right.is_int()) {
    return std::get<long long>(left.v) == std::get<long long>(right.v);
  }
  return false;
}

expr::Value Interpreter::visit_unary_expr(const expr::Unary &expr)
{
  expr::Value right = evaluate(*expr.right);

  switch(expr.op.get_type()) {
  case Token::TokenType::MINUS:
    check_number_operand(expr.op, right);

    if(right.is_int()) {
      return -std::get<long long>(right.v);
    }
    else {
      return -std::get<double>(right.v);
    }

  case Token::TokenType::BANG: return !is_truthy(right);

  default:
    // unreachable
    return {};
  }
}

void Interpreter::check_number_operand(const Token &op, const expr::Value &operand)
{
  if(operand.is_number()) {
    return;
  }
  throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token &op, const expr::Value &left,
                                        const expr::Value &right)
{
  if(left.is_number() && right.is_number()) {
    return;
  }
  throw RuntimeError(op, "Operands must be numbers.");
}

std::string Interpreter::stringify(const expr::Value &value)
{
  if(value.is_nil()) {
    return "nil";
  }
  else if(value.is_double()) {
    return std::to_string(value.as<double>());
  }
  else if(value.is_int()) {
    return std::to_string(value.as<long long>());
  }
  else if(value.is_string()) {
    return value.as<std::string>();
  }
  else if(value.is_bool()) {
    return value.as<bool>() ? "true" : "false";
  }
  else if(value.is_callable()) {
    return value.as<std::shared_ptr<LoxCallable>>()->to_string();
  }
  else if(value.is_instance()) {
    return value.as<std::shared_ptr<LoxInstance>>()->to_string();
  }
  return "???unknown???";
}

void Interpreter::visit_print_stmt(const stmt::Print &stmt)
{
  auto value = evaluate(*stmt.ex.get());
  std::cout << stringify(value) << std::endl;
}

void Interpreter::visit_expr_stmt(const stmt::Expression &stmt)
{
  show_exp = true;
  auto v = evaluate(*stmt.ex.get());
  if(repl && show_exp) {
    std::cout << stringify(v) << std::endl;
  }
}

expr::Value Interpreter::visit_variable_expr(const std::shared_ptr<const expr::Variable> &expr)
{
  return lookup_variable(expr->token, expr);
}

expr::Value Interpreter::visit_assignment_expr(const std::shared_ptr<const expr::Assignment> &expr)
{
  show_exp = false;
  auto value = evaluate(*expr->value);

  if(locals.find(expr) != locals.end()) {
    environ->assign_at(locals[expr], expr->token.get_lexeme(), value);
  }
  else {
    globals->assign(expr->token, value);
  }
  return value;
}

expr::Value Interpreter::visit_logical_expr(const expr::Logical &expr)
{
  auto left = evaluate(*expr.left);
  if(expr.op.get_type() == Token::TokenType::OR) {
    if(is_truthy(left)) {
      return left;
    }
  }
  else {
    // AND
    if(!is_truthy(left)) {
      return left;
    }
  }
  return evaluate(*expr.right);
}

expr::Value Interpreter::visit_call_expr(const expr::Call &expr)
{
  show_exp = false;
  auto callee = evaluate(*expr.callee);
  auto args = std::vector<expr::Value>{};
  for(const auto &arg : expr.arguments) {
    args.push_back(evaluate(*arg));
  }

  if(!callee.is_callable()) {
    throw RuntimeError(expr.paren, "Can only call functions and classes.");
  }

  auto &func = *callee.as<std::shared_ptr<LoxCallable>>();
  if(func.arity() != args.size()) {
    throw RuntimeError(expr.paren, "Expected " + std::to_string(func.arity())
                                     + " arguments but got " + std::to_string(args.size()));
  }
  return func.call(*this, args);
}

void Interpreter::visit_vardecl_stmt(const stmt::VariableDecl &stmt)
{
  // by default, if a variable declaration has no initializer, the value is nil
  show_exp = false;
  expr::Value value{};
  if(stmt.initializer != nullptr) {
    value = evaluate(*stmt.initializer);
  }

  environ->define(stmt.token.get_lexeme(), value);
}

void Interpreter::visit_block_stmt(const stmt::Block &stmt)
{
  execute_block(stmt.statements, std::make_unique<Environment>(environ));
}

void Interpreter::visit_if_stmt(const stmt::If &stmt)
{
  if(is_truthy(evaluate(*stmt.condition))) {
    execute(stmt.then_stm);
  }
  else if(stmt.else_stm != nullptr) {
    execute(std::shared_ptr<stmt::StmtBase>(stmt.else_stm));
  }
}

void Interpreter::visit_while_stmt(const stmt::While &stmt)
{
  while(is_truthy(evaluate(*stmt.condition))) {
    execute(stmt.body);
  }
}

void Interpreter::visit_fun_stmt(const std::shared_ptr<const stmt::Function> &stmt)
{
  auto func = std::make_shared<LoxFunction>(stmt, environ, false);
  environ->define(stmt->name.get_lexeme(), expr::Value(func));
}

void Interpreter::visit_return_stmt(const stmt::Return &stmt)
{
  if(stmt.value != nullptr) {
    throw Return(evaluate(*stmt.value));
  }
  // by default 'return;' returns nil
  throw Return(expr::Value());
}

void Interpreter::visit_class_stmt(const std::shared_ptr<const stmt::Class> &stmt)
{
  // validate the superclass if any
  std::shared_ptr<const LoxClass> superclass{};
  if(stmt->superclass != nullptr) {
    auto super = evaluate(*stmt->superclass);
    if(!super.is_callable()
       || typeid(*super.as<std::shared_ptr<LoxCallable>>()) != typeid(LoxClass)) {
      throw RuntimeError(stmt->superclass->token, "Superclass must be a class.");
    }
    superclass = std::dynamic_pointer_cast<const LoxClass>(super.as<std::shared_ptr<LoxCallable>>());
  }

  std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods;
  for(auto &method : stmt->methods) {
    // each method is turned into the runtime representation
    auto lexeme = method->name.get_lexeme();
    methods[lexeme] = std::make_shared<LoxFunction>(method, environ, lexeme == "init");
  }

  auto klass = std::make_shared<LoxClass>(stmt->name.get_lexeme(), std::move(superclass), std::move(methods));
  environ->define(stmt->name.get_lexeme(), expr::Value(klass));
}

expr::Value Interpreter::visit_get_expr(const expr::Get &expr)
{
  auto v = evaluate(*expr.object);
  if(v.is_instance()) {
    return v.as<std::shared_ptr<LoxInstance>>()->get(expr.name);
  }
  throw RuntimeError(expr.name, "Only instances have properties.");
}

expr::Value Interpreter::visit_set_expr(const expr::Set &expr)
{
  show_exp = false;
  auto object = evaluate(*expr.object);
  if(object.is_instance()) {
    auto &instance = object.as<std::shared_ptr<LoxInstance>>();
    auto value = evaluate(*expr.value);
    instance->set(expr.name, value);
    return value;
  }
  throw RuntimeError(expr.name, "Only instances have fields.");
}

expr::Value Interpreter::visit_this_expr(const std::shared_ptr<const expr::This> &expr)
{
  return lookup_variable(expr->token, expr);
}

void Interpreter::execute_block(const std::vector<std::shared_ptr<stmt::StmtBase>> &stmts,
                                std::unique_ptr<Environment> env)
{
  auto previous = environ;
  try {
    environ = std::move(env);
    for(auto &stm : stmts) {
      execute(stm);
    }
  }
  catch(...) {
    environ = previous;
    throw;
  }
  environ = previous;
}

void Interpreter::interpret(const std::vector<std::shared_ptr<stmt::StmtBase>> &stms, bool repl)
{
  this->repl = repl;
  try {
    for(auto &stm : stms) {
      execute(stm);
    }
  }
  catch(const RuntimeError &error) {
    Lox::runtime_error(error);
  }
}

expr::Value Interpreter::lookup_variable(const Token &name,
                                         const std::shared_ptr<const expr::ExprBase> &expr)
{
  if(locals.find(expr) == locals.end()) {
    return globals->get(name);
  }
  else {
    return environ->get(name);
  }
}

void Interpreter::define_native_functions()
{
  class NativeFunctionClock : public LoxCallable
  {
    expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override
    {
      const auto now = std::chrono::system_clock::now();
      long long milliseconds_since_epoch
        = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
      return expr::Value(milliseconds_since_epoch);
    }
    int arity() const override { return 0; }
    std::string to_string() const override { return "<native fn>"; }
  };

  auto f = std::make_shared<NativeFunctionClock>();
  globals->define("clock", expr::Value(f));
}