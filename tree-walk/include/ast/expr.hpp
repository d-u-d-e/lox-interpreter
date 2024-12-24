#pragma once

#include <visitor.hpp>
#include <token.hpp>
#include <memory>

namespace expr
{

    using value = std::variant<std::string, double, std::nullptr_t, bool>;

    class ExprBase
    {
    public:
        virtual ~ExprBase() = default;
        virtual std::string accept(Visitor<std::string> &visitor) const = 0;
        virtual expr::value accept(expr::Visitor<expr::value> &visitor) const = 0;
    };

    struct Binary : public ExprBase
    {
        Binary(std::unique_ptr<ExprBase> &&left, const Token &op, std::unique_ptr<ExprBase> &&right) : left(std::move(left)), op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_binary_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_binary_expr(*this); }
        std::unique_ptr<ExprBase> left;
        Token op;
        std::unique_ptr<ExprBase> right;
    };

    struct Grouping : public ExprBase
    {
        Grouping(std::unique_ptr<ExprBase> &&expr) : expr(std::move(expr)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_grouping_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_grouping_expr(*this); }
        std::unique_ptr<ExprBase> expr;
    };

    struct Literal : public ExprBase
    {
        Literal(const Token::Literal &l)
        {
            if (std::holds_alternative<std::string>(l))
            {
                value = std::get<std::string>(l);
            }
            else if (std::holds_alternative<double>(l))
            {
                value = std::get<double>(l);
            }
            else if (std::holds_alternative<bool>(l))
            {
                value = std::get<bool>(l);
            }
            else
            {
                value = nullptr;
            }
        }
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_literal_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_literal_expr(*this); }
        expr::value value;
    };

    struct Logical : public ExprBase
    {
        Logical(std::unique_ptr<ExprBase> &&left, const Token &op, std::unique_ptr<ExprBase> &&right) : left(std::move(left)), op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_logical_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_logical_expr(*this); }
        std::unique_ptr<ExprBase> left;
        Token op;
        std::unique_ptr<ExprBase> right;
    };

    struct Unary : public ExprBase
    {
        Unary(const Token &op, std::unique_ptr<ExprBase> &&right) : op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_unary_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_unary_expr(*this); }
        Token op;
        std::unique_ptr<ExprBase> right;
    };

    struct Variable : public ExprBase
    {
        Variable(const Token &token) : token(token) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_variable_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_variable_expr(*this); }
        Token token;
    };

    struct Assignment : public ExprBase
    {
        Assignment(const Token &token, std::unique_ptr<ExprBase> && value) : token(token), value(std::move(value)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_assignment_expr(*this); }
        expr::value accept(Visitor<expr::value> &visitor) const override { return visitor.visit_assignment_expr(*this); }
        Token token;
        std::unique_ptr<ExprBase> value;
    };

}
