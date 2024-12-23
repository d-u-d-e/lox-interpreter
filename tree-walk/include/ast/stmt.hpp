#pragma once

#include <visitor.hpp>
#include <ast/expr.hpp>

namespace stmt
{

    class StmtBase
    {
    public:
        virtual ~StmtBase() = default;
        virtual void accept(Visitor<void> &visitor) const = 0;
    };

    // This is an expression statement,
    // i.e. an expression followed by a semicolon
    struct Expression : public StmtBase
    {
        Expression(std::unique_ptr<expr::ExprBase> &&ex) : ex(std::move(ex)) {}
        void accept(Visitor<void> &visitor) const override
        {
            visitor.visit_expr_stmt(*this);
        }
        std::unique_ptr<expr::ExprBase> ex;
    };

    struct Print : public StmtBase
    {
        Print(std::unique_ptr<expr::ExprBase> &&ex) : ex(std::move(ex)) {}
        void accept(Visitor<void> &visitor) const override
        {
            visitor.visit_print_stmt(*this);
        }
        std::unique_ptr<expr::ExprBase> ex;
    };
}
