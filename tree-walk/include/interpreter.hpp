#pragma once
#include <ast/expr.hpp>
#include <ast/stmt.hpp>
#include <iostream>
#include <vector>
#include <environment.hpp>

class Interpreter : public expr::Visitor<expr::value>,
                    public stmt::Visitor<void>
{
public:
    Interpreter()
    {
        // global env
        env = std::make_shared<Environment>();
    }

    struct RuntimeError : public std::runtime_error
    {
        RuntimeError(const Token &token, const std::string &message) : std::runtime_error(message), token(token) {}
        Token token;
    };
    expr::value visit_binary_expr(const expr::Binary &expr) override;
    expr::value visit_grouping_expr(const expr::Grouping &expr) override;
    expr::value visit_literal_expr(const expr::Literal &expr) override;
    expr::value visit_unary_expr(const expr::Unary &expr) override;
    expr::value visit_variable_expr(const expr::Variable &expr) override;
    expr::value visit_assignment_expr(const expr::Assignment &expr) override;
    expr::value visit_logical_expr(const expr::Logical &expr) override;

    void visit_print_stmt(const stmt::Print &stmt) override;
    void visit_expr_stmt(const stmt::Expression &stmt) override;
    void visit_vardecl_stmt(const stmt::VariableDecl &stmt) override;
    void visit_block_stmt(const stmt::Block &stmt) override;
    void visit_if_stmt(const stmt::If &stmt) override;

    void interpret(const std::vector<std::unique_ptr<stmt::StmtBase>> &stms, bool repl = false);
    static std::string stringify(const expr::value &value);

private:
    void check_number_operand(const Token &op, const expr::value &operand);
    void check_number_operands(const Token &op, const expr::value &left, const expr::value &right);

    bool is_truthy(const expr::value &value);
    bool is_equal(const expr::value &left, expr::value &right);
    expr::value evaluate(const expr::ExprBase &expr) { return expr.accept(*this); }
    void execute(const stmt::StmtBase &stmt) { return stmt.accept(*this); }
    void execute_block(const std::vector<std::unique_ptr<stmt::StmtBase>> &stmts, std::unique_ptr<Environment> environ);

    std::shared_ptr<Environment> env;
    bool repl{false};
};