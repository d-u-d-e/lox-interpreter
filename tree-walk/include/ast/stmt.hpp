#pragma once

#include <visitor.hpp>
#include <ast/expr.hpp>
#include <vector>

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

    struct VariableDecl : public StmtBase
    {
        VariableDecl(const Token &token, std::unique_ptr<expr::ExprBase> &&initializer) : token(token), initializer(std::move(initializer)) {}
        void accept(Visitor<void> &visitor) const override
        {
            visitor.visit_vardecl_stmt(*this);
        }
        Token token;
        std::unique_ptr<expr::ExprBase> initializer;
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

    struct Block : public StmtBase
    {
        Block(std::vector<std::unique_ptr<StmtBase>> &&statements) : statements(std::move(statements)) {}
        void accept(Visitor<void> &visitor) const override
        {
            visitor.visit_block_stmt(*this);
        }
        std::vector<std::unique_ptr<stmt::StmtBase>> statements;
    };
}
