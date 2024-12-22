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
        T virtual visit_binary(const Binary &expr) = 0;
        T virtual visit_grouping(const Grouping &expr) = 0;
        T virtual visit_literal(const Literal &expr) = 0;
        T virtual visit_unary(const Unary &expr) = 0;
    };
}
