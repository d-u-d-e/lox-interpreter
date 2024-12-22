#pragma once

#include <visitor.hpp>
#include <token.hpp>
#include <memory>

namespace expr
{
    class ExprBase
    {
    public:
        virtual ~ExprBase() = default;
        virtual std::string accept(Visitor<std::string> &visitor) const = 0;
    };

    struct Binary : public ExprBase
    {
        Binary(std::unique_ptr<ExprBase> && left, const Token &token, std::unique_ptr<ExprBase> && right) : left(std::move(left)), token(token), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visitBinary(*this); }
        std::unique_ptr<ExprBase> left;
        Token token;
        std::unique_ptr<ExprBase> right;
    };

    struct Grouping : public ExprBase
    {
        Grouping(std::unique_ptr<ExprBase> && expr) : expr(std::move(expr)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visitGrouping(*this); }
        std::unique_ptr<ExprBase> expr;
    };

    struct Literal : public ExprBase
    {
        Literal(const Token::Literal &literal) : literal(literal) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visitLiteral(*this); }
        Token::Literal literal;
    };

    struct Unary : public ExprBase
    {
        Unary(const Token &token, std::unique_ptr<ExprBase> && right) : token(token), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visitUnary(*this); }
        Token token;
        std::unique_ptr<ExprBase> right;
    };
}
