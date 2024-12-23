#pragma once

namespace expr
{
    class Binary;
    class Grouping;
    class Literal;
    class Unary;

    template <typename T>
    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        T virtual visit_binary_expr(const Binary &expr) = 0;
        T virtual visit_grouping_expr(const Grouping &expr) = 0;
        T virtual visit_literal_expr(const Literal &expr) = 0;
        T virtual visit_unary_expr(const Unary &expr) = 0;
    };
}

namespace stmt
{
    class Print;
    class Expression;

    template <typename T>
    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        T virtual visit_print_stmt(const Print &expr) = 0;
        T virtual visit_expr_stmt(const Expression &expr) = 0;
    };
}