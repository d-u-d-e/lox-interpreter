#include <resolver.hpp>
#include <lox.hpp>

void Resolver::declare(const Token &token)
{
    if (scopes.empty())
    {
        // we are in the global scope
        return;
    }

    // we are in a local scope
    auto &scope_top = scopes.at(scopes.size() - 1);
    auto lexeme = token.get_lexeme();
    if (scope_top.find(lexeme) != scope_top.end())
    {
        Lox::error(token, "Already a variable with this name in this scope.");
    }
    // mark the variable as not ready yet
    scope_top[lexeme] = false;
}

void Resolver::define(const Token &token)
{
    if (scopes.empty())
        return;
    // mark the variable as ready
    scopes.at(scopes.size() - 1).at(token.get_lexeme()) = true;
}

void Resolver::visit_binary_expr(const expr::Binary &expr)
{
    resolve(expr.left);
    resolve(expr.right);
}

void Resolver::visit_grouping_expr(const expr::Grouping &expr)
{
    resolve(expr.expr);
};

void Resolver::visit_literal_expr(const expr::Literal &expr) {
    // empty
};

void Resolver::visit_unary_expr(const expr::Unary &expr)
{
    resolve(expr.right);
};

void Resolver::visit_variable_expr(const std::shared_ptr<expr::Variable> &expr)
{
    auto &token = expr.get()->token;
    if (!scopes.empty())
    {
        auto &scope_top = scopes.at(scopes.size() - 1);
        if (scope_top.find(token.get_lexeme()) != scope_top.end() && !scope_top[token.get_lexeme()])
        {
            Lox::error(token, "Can't read local variable in its own initializer");
        }
    }
    resolve_local(expr, token);
};

void Resolver::visit_assignment_expr(const std::shared_ptr<expr::Assignment> &expr)
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
    for (const auto &arg : expr.arguments)
    {
        resolve(arg);
    }
};

void Resolver::visit_print_stmt(stmt::Print &stmt)
{
    resolve(stmt.ex);
};

void Resolver::visit_expr_stmt(stmt::Expression &stmt)
{
    resolve(stmt.ex);
};

void Resolver::visit_vardecl_stmt(stmt::VariableDecl &stmt)
{
    // variable exists but has not been initialized
    // a statement like var a = a; is a compile error, as 'a' is not ready
    declare(stmt.token);
    if (stmt.initializer != nullptr)
    {
        resolve(stmt.initializer);
    }
    // variable has been initialized, so it is ready
    define(stmt.token);
};

void Resolver::visit_block_stmt(stmt::Block &stmt)
{
    begin_scope();
    resolve(stmt.statements);
    end_scope();
};

void Resolver::visit_if_stmt(stmt::If &stmt)
{
    resolve(stmt.condition);
    resolve(stmt.then_stm);
    if (stmt.else_stm != nullptr)
    {
        resolve(stmt.else_stm);
    }
};

void Resolver::visit_while_stmt(std::shared_ptr<stmt::While> stmt)
{
    resolve(stmt->condition);
    resolve(stmt->body);
};

void Resolver::visit_fun_stmt(std::shared_ptr<stmt::Function> stmt)
{
    declare(stmt->name);
    define(stmt->name);
    // this lets a function recursively refer to itself inside its own body
    resolve_function(stmt, FunctionType::FUNCTION);
};

void Resolver::visit_return_stmt(stmt::Return &stmt)
{
    if (current_func == FunctionType::NONE)
    {
        Lox::error(stmt.keyword, "Can't return from top-level code.");
    }

    if (stmt.value != nullptr)
    {
        resolve(stmt.value);
    }
};

void Resolver::resolve(const std::vector<std::shared_ptr<stmt::StmtBase>> &statements)
{
    for (auto &stm : statements)
    {
        resolve(stm);
    }
}

void Resolver::resolve_local(const std::shared_ptr<expr::ExprBase> &expr, const Token &token)
{
    for (int i = scopes.size() - 1; i >= 0; i--)
    {
        if (scopes[i].find(token.get_lexeme()) != scopes[i].end())
        {
            interpreter.resolve(expr, scopes.size() - i - 1);
            return;
        }
    } 
}

void Resolver::resolve_function(const std::shared_ptr<stmt::Function> &function, FunctionType type)
{
    // used to resolve both functions and methods

    FunctionType enclosing_func = current_func;
    current_func = type;

    begin_scope();
    for (auto &param : function->params)
    {
        declare(param);
        define(param);
    }
    resolve(function->body);
    end_scope();

    current_func = enclosing_func;
}