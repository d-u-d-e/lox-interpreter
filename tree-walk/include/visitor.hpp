#pragma once

namespace expr
{
    class Binary;
    class Grouping;
    class Literal;
    class Unary;
    class Variable;

    template <typename T>
    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        T virtual visit_binary_expr(const Binary &expr) = 0;
        T virtual visit_grouping_expr(const Grouping &expr) = 0;
        T virtual visit_literal_expr(const Literal &expr) = 0;
        T virtual visit_unary_expr(const Unary &expr) = 0;
        T virtual visit_variable_expr(const Variable &expr) = 0;
    };
}

namespace stmt
{
    class Print;
    class Expression;
    class VariableDecl;

    template <typename T>
    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        T virtual visit_print_stmt(const Print &stmt) = 0;
        T virtual visit_expr_stmt(const Expression &stmt) = 0;
        T virtual visit_vardecl_stmt(const VariableDecl &stmt) = 0;
    };
}