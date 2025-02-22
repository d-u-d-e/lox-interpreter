#include <lox.hpp>
#include <resolver.hpp>

void Resolver::declare(const Token &token)
{
  if(scopes.empty()) {
    // we are in the global scope
    return;
  }

  // we are in a local scope
  auto &scope_top = scopes.back();
  auto lexeme = token.get_lexeme();
  if(scope_top.find(lexeme) != scope_top.end()) {
    Lox::error(token, "Already a variable with this name in this scope.");
  }
  // mark the variable as not ready yet
  scope_top[lexeme] = false;
}

void Resolver::define(const Token &token)
{
  if(scopes.empty())
    return;
  // mark the variable as ready
  scopes.back()[token.get_lexeme()] = true;
}

void Resolver::visit_binary_expr(const expr::Binary &expr)
{
  resolve(expr.left);
  resolve(expr.right);
}

void Resolver::visit_grouping_expr(const expr::Grouping &expr) { resolve(expr.expr); };

void Resolver::visit_literal_expr(const expr::Literal &expr) {
  // empty
};

void Resolver::visit_unary_expr(const expr::Unary &expr) { resolve(expr.right); };

void Resolver::visit_variable_expr(const std::shared_ptr<const expr::Variable> &expr)
{
  auto &token = expr.get()->token;
  if(!scopes.empty()) {
    auto &scope_top = scopes.back();
    if(scope_top.find(token.get_lexeme()) != scope_top.end() && !scope_top[token.get_lexeme()]) {
      Lox::error(token, "Can't read local variable in its own initializer.");
    }
  }
  resolve_local(expr, token);
};

void Resolver::visit_assignment_expr(const std::shared_ptr<const expr::Assignment> &expr)
{
  resolve(expr->value);
  resolve_local(expr, expr->token);
};

void Resolver::visit_logical_expr(const expr::Logical &expr)
{
  resolve(expr.left);
  resolve(expr.right);
};

void Resolver::visit_call_expr(const expr::Call &expr)
{
  resolve(expr.callee);
  for(const auto &arg : expr.arguments) {
    resolve(arg);
  }
};

void Resolver::visit_get_expr(const expr::Get &expr)
{
  // properties are dynamic, so they don't get resolved
  resolve(expr.object);
}

void Resolver::visit_set_expr(const expr::Set &expr)
{
  // properties are dynamic, so they don't get resolved
  resolve(expr.value);
  resolve(expr.object);
}

void Resolver::visit_this_expr(const std::shared_ptr<const expr::This> &expr)
{
  if(current_class == ClassType::NONE) {
    Lox::error(expr->token, "Can't use 'this' outside of a class.");
    return;
  }

  // treated as a normal variable
  resolve_local(expr, expr->token);
}

void Resolver::visit_super_expr(const std::shared_ptr<const expr::Super> &expr)
{
  if(current_class == ClassType::NONE) {
    Lox::error(expr->keyword, "Can't use 'super' outside of a class.");
  }
  else if(current_class != ClassType::SUBCLASS) {
    Lox::error(expr->keyword, "Can't use 'super' in a class with no superclass.");
  }
  resolve_local(expr, expr->keyword);
}

void Resolver::visit_print_stmt(const stmt::Print &stmt) { resolve(stmt.ex); };

void Resolver::visit_expr_stmt(const stmt::Expression &stmt) { resolve(stmt.ex); };

void Resolver::visit_vardecl_stmt(const stmt::VariableDecl &stmt)
{
  // variable exists but has not been initialized
  // a statement like var a = a; is a compile error, as 'a' is not ready
  declare(stmt.token);
  if(stmt.initializer != nullptr) {
    resolve(stmt.initializer);
  }
  // variable has been initialized, so it is ready
  define(stmt.token);
};

void Resolver::visit_block_stmt(const stmt::Block &stmt)
{
  begin_scope();
  resolve(stmt.statements);
  end_scope();
};

void Resolver::visit_if_stmt(const stmt::If &stmt)
{
  resolve(stmt.condition);
  resolve(stmt.then_stm);
  if(stmt.else_stm != nullptr) {
    resolve(stmt.else_stm);
  }
};

void Resolver::visit_while_stmt(const stmt::While &stmt)
{
  resolve(stmt.condition);
  resolve(stmt.body);
};

void Resolver::visit_fun_stmt(const std::shared_ptr<const stmt::Function> &stmt)
{
  declare(stmt->name);
  define(stmt->name);
  // this lets a function recursively refer to itself inside its own body
  resolve_function(stmt, FunctionType::FUNCTION);
};

void Resolver::visit_return_stmt(const stmt::Return &stmt)
{
  if(current_func == FunctionType::NONE) {
    Lox::error(stmt.keyword, "Can't return from top-level code.");
  }

  // do not allow returning a value from an initializer
  // allow 'return;' only

  if(stmt.value != nullptr) {
    if(current_func == FunctionType::INITIALIZER) {
      Lox::error(stmt.keyword, "Can't return a value from an initializer.");
    }
    resolve(stmt.value);
  }
};

void Resolver::visit_class_stmt(const std::shared_ptr<const stmt::Class> &stmt)
{
  auto enclosing = current_class;
  current_class = ClassType::CLASS;

  declare(stmt->name);
  // not uncommon to declare a class as a local variable
  define(stmt->name);

  // a class can't inherit from itself
  if(stmt->superclass != nullptr
     && stmt->name.get_lexeme() == stmt->superclass->token.get_lexeme()) {
    Lox::error(stmt->superclass->token, "A class can't inherit from itself.");
  }

  // resolve the superclass
  if(stmt->superclass != nullptr) {
    current_class = ClassType::SUBCLASS;
    resolve(stmt->superclass);
    // if the class decl has a superclass, we create a scope for it sorrouding all of its methods,
    // where we define `super`
    begin_scope();
    scopes.back()["super"] = true;
  }

  // make `this` visible to the function bodies
  begin_scope();
  scopes.back()["this"] = true;

  // resolve the methods
  for(auto &method : stmt->methods) {
    auto func_type
      = method->name.get_lexeme() == "init" ? FunctionType::INITIALIZER : FunctionType::METHOD;
    resolve_function(method, func_type);
  }

  end_scope(); // `this` is no longer visible

  if(stmt->superclass != nullptr) {
    end_scope(); // `super` is no longer visible
  }

  current_class = enclosing;
}

void Resolver::resolve(const std::vector<std::shared_ptr<stmt::StmtBase>> &statements)
{
  for(auto &stm : statements) {
    resolve(stm);
  }
}

void Resolver::resolve_local(const std::shared_ptr<const expr::ExprBase> &expr, const Token &token)
{
  for(int i = scopes.size() - 1; i >= 0; i--) {
    if(scopes[i].find(token.get_lexeme()) != scopes[i].end()) {
      interpreter.resolve(expr, scopes.size() - i - 1);
      return;
    }
  }
}

void Resolver::resolve_function(const std::shared_ptr<const stmt::Function> &function,
                                FunctionType type)
{
  // used to resolve both functions and methods

  FunctionType enclosing_func = current_func;
  current_func = type;

  begin_scope();
  for(auto &param : function->params) {
    declare(param);
    define(param);
  }
  resolve(function->body);
  end_scope();

  current_func = enclosing_func;
}