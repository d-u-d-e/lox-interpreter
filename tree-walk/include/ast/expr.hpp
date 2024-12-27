#pragma once

#include <visitor.hpp>
#include <token.hpp>
#include <memory>
#include <function.hpp>

namespace expr
{
    struct Value
    {
    public:
        virtual ~Value() = default;

        Value(const std::string &s) : v(s) {}
        Value(double d) : v(d) {}
        Value(bool b) : v(b) {}
        Value(LoxFunction f) : v(f) {}
        Value() = default;

        template <typename T>
        T as() const
        {
            return std::get<T>(v);
        }
        template <typename T>
        T &as()
        {
            return std::get<T>(v);
        }
        bool is_string() const { return std::holds_alternative<std::string>(v); }
        bool is_double() const { return std::holds_alternative<double>(v); }
        bool is_bool() const { return std::holds_alternative<bool>(v); }
        bool is_nil() const { return std::holds_alternative<std::monostate>(v); }
        bool is_callable() const { return std::holds_alternative<LoxFunction>(v); };
        std::variant<std::monostate, std::string, double, bool, LoxFunction> v{};
    };

    class ExprBase
    {
    public:
        virtual ~ExprBase() = default;
        virtual std::string accept(Visitor<std::string> &visitor) const = 0;
        virtual Value accept(expr::Visitor<Value> &visitor) const = 0;
    };

    struct Binary : public ExprBase
    {
        Binary(std::shared_ptr<ExprBase> &&left, const Token &op, std::shared_ptr<ExprBase> &&right) : left(std::move(left)), op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_binary_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_binary_expr(*this); }
        std::shared_ptr<ExprBase> left;
        Token op;
        std::shared_ptr<ExprBase> right;
    };

    struct Call : public ExprBase
    {
        Call(std::shared_ptr<ExprBase> &&callee, const Token &paren, std::vector<std::shared_ptr<ExprBase>> &&arguments) : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_call_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_call_expr(*this); }
        std::shared_ptr<ExprBase> callee;
        Token paren;
        std::vector<std::shared_ptr<ExprBase>> arguments;
    };

    struct Grouping : public ExprBase
    {
        Grouping(std::shared_ptr<ExprBase> &&expr) : expr(std::move(expr)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_grouping_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_grouping_expr(*this); }
        std::shared_ptr<ExprBase> expr;
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
        }
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_literal_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_literal_expr(*this); }
        Value value;
    };

    struct Logical : public ExprBase
    {
        Logical(std::shared_ptr<ExprBase> &&left, const Token &op, std::shared_ptr<ExprBase> &&right) : left(std::move(left)), op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_logical_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_logical_expr(*this); }
        std::shared_ptr<ExprBase> left;
        Token op;
        std::shared_ptr<ExprBase> right;
    };

    struct Unary : public ExprBase
    {
        Unary(const Token &op, std::shared_ptr<ExprBase> &&right) : op(op), right(std::move(right)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_unary_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_unary_expr(*this); }
        Token op;
        std::shared_ptr<ExprBase> right;
    };

    struct Variable : public ExprBase
    {
        Variable(const Token &token) : token(token) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_variable_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_variable_expr(*this); }
        Token token;
    };

    struct Assignment : public ExprBase
    {
        Assignment(const Token &token, std::shared_ptr<ExprBase> &&value) : token(token), value(std::move(value)) {}
        std::string accept(Visitor<std::string> &visitor) const override { return visitor.visit_assignment_expr(*this); }
        Value accept(Visitor<Value> &visitor) const override { return visitor.visit_assignment_expr(*this); }
        Token token;
        std::shared_ptr<ExprBase> value;
    };

}
