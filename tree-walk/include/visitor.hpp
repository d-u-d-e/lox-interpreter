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
        T virtual visitBinary(const Binary &expr) = 0;
        T virtual visitGrouping(const Grouping &expr) = 0;
        T virtual visitLiteral(const Literal &expr) = 0;
        T virtual visitUnary(const Unary &expr) = 0;
    };
}
