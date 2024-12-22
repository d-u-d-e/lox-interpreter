#pragma once
#include <ast/expr.hpp>
#include <iostream>

class Interpreter : public expr::Visitor<expr::value>
{
public:
    struct RuntimeError : public std::runtime_error
    {
        RuntimeError(const Token &token, const std::string &message) : std::runtime_error(message), token(token) {}
        Token token;
    };
    expr::value visit_binary(const expr::Binary &expr) override;
    expr::value visit_grouping(const expr::Grouping &expr) override;
    expr::value visit_literal(const expr::Literal &expr) override;
    expr::value visit_unary(const expr::Unary &expr) override;

    void interpret(const expr::ExprBase &expr);


private:

    std::string stringify(const expr::value &value);

    void check_number_operand(const Token &op, const expr::value &operand);
    void check_number_operands(const Token &op, const expr::value &left, const expr::value &right);

    bool is_truthy(const expr::value &value);
    bool is_equal(const expr::value &left, expr::value &right);
    expr::value evaluate(const expr::ExprBase &expr) { return expr.accept(*this); }
};