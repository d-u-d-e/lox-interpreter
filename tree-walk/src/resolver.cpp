#include <resolver.hpp>
#include <ast/stmt.hpp>

void Resolver::visit_binary_expr(const expr::Binary &expr)
{
}

void Resolver::visit_grouping_expr(const expr::Grouping &expr) {};
void Resolver::visit_literal_expr(const expr::Literal &expr) {};
void Resolver::visit_unary_expr(const expr::Unary &expr) {};
void Resolver::visit_variable_expr(const expr::Variable &expr) {};
void Resolver::visit_assignment_expr(const expr::Assignment &expr) {};
void Resolver::visit_logical_expr(const expr::Logical &expr) {};
void Resolver::visit_call_expr(const expr::Call &expr) {};

void Resolver::visit_print_stmt(stmt::Print &stmt) {};
void Resolver::visit_expr_stmt(stmt::Expression &stmt) {};
void Resolver::visit_vardecl_stmt(stmt::VariableDecl &stmt) {};
void Resolver::visit_block_stmt(stmt::Block &stmt)
{
    begin_scope();
    resolve(stmt.statements);
    end_scope();
};

void Resolver::visit_if_stmt(stmt::If &stmt) {};
void Resolver::visit_while_stmt(std::shared_ptr<stmt::While> stmt) {};
void Resolver::visit_fun_stmt(std::shared_ptr<stmt::Function> stmt) {};
void Resolver::visit_return_stmt(stmt::Return &stmt) {};

void Resolver::resolve(std::vector<std::shared_ptr<stmt::StmtBase>> &statements)
{

}