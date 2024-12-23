#pragma once
#include <ast/expr.hpp>
#include <ast/stmt.hpp>
#include <iostream>
#include <vector>

class Interpreter : public expr::Visitor<expr::value>,
                    public stmt::Visitor<void>
{
public:
    struct RuntimeError : public std::runtime_error
    {
        RuntimeError(const Token &token, const std::string &message) : std::runtime_error(message), token(token) {}
        Token token;
    };
    expr::value visit_binary_expr(const expr::Binary &expr) override;
    expr::value visit_grouping_expr(const expr::Grouping &expr) override;
    expr::value visit_literal_expr(const expr::Literal &expr) override;
    expr::value visit_unary_expr(const expr::Unary &expr) override;

    void visit_print_stmt(const stmt::Print &stmt) override;
    void visit_expr_stmt(const stmt::Expression &stmt) override;

    void interpret(const std::vector<std::unique_ptr<stmt::StmtBase>> &stms);

private:
    std::string stringify(const expr::value &value);

    void check_number_operand(const Token &op, const expr::value &operand);
    void check_number_operands(const Token &op, const expr::value &left, const expr::value &right);

    bool is_truthy(const expr::value &value);
    bool is_equal(const expr::value &left, expr::value &right);
    expr::value evaluate(const expr::ExprBase &expr) { return expr.accept(*this); }
    void execute(const stmt::StmtBase &stmt) { return stmt.accept(*this); }
};