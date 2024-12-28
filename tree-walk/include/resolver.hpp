#pragma once
#include <visitor.hpp>
#include <ast/expr.hpp>
#include <stack>
#include <unordered_map>

namespace stmt
{
    class StmtBase;
};

class Resolver : public expr::Visitor<void>, public stmt::Visitor<void>
{
public:
    void visit_binary_expr(const expr::Binary &expr) override;
    void visit_grouping_expr(const expr::Grouping &expr) override;
    void visit_literal_expr(const expr::Literal &expr) override;
    void visit_unary_expr(const expr::Unary &expr) override;
    void visit_variable_expr(const expr::Variable &expr) override;
    void visit_assignment_expr(const expr::Assignment &expr) override;
    void visit_logical_expr(const expr::Logical &expr) override;
    void visit_call_expr(const expr::Call &expr) override;

    void visit_print_stmt(stmt::Print &stmt) override;
    void visit_expr_stmt(stmt::Expression &stmt) override;
    void visit_vardecl_stmt(stmt::VariableDecl &stmt) override;
    void visit_block_stmt(stmt::Block &stmt) override;
    void visit_if_stmt(stmt::If &stmt) override;
    void visit_while_stmt(std::shared_ptr<stmt::While> stmt) override;
    void visit_fun_stmt(std::shared_ptr<stmt::Function> stmt) override;
    void visit_return_stmt(stmt::Return &stmt) override;

private:
    std::stack<std::unordered_map<std::string, bool>> scopes;
    void begin_scope()
    {
        scopes.push(std::unordered_map<std::string, bool>());
    }
    void end_scope()
    {
        scopes.pop();
    }
    void resolve(std::vector<std::shared_ptr<stmt::StmtBase>> &statements);
};