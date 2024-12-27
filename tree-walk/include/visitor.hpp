#pragma once
#include <memory>

namespace expr
{
    class Binary;
    class Grouping;
    class Literal;
    class Unary;
    class Variable;
    class Assignment;
    class Logical;
    class Call;

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
        T virtual visit_assignment_expr(const Assignment &expr) = 0;
        T virtual visit_logical_expr(const Logical &expr) = 0;
        T virtual visit_call_expr(const Call &expr) = 0;
    };
}

namespace stmt
{
    class Print;
    class Expression;
    class VariableDecl;
    class Block;
    class If;
    class While;
    class Function;
    class Return;

    template <typename T>
    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        T virtual visit_print_stmt(Print &stmt) = 0;
        T virtual visit_expr_stmt(Expression &stmt) = 0;
        T virtual visit_vardecl_stmt(VariableDecl &stmt) = 0;
        T virtual visit_block_stmt(Block &stmt) = 0;
        T virtual visit_if_stmt(If &stmt) = 0;
        T virtual visit_while_stmt(std::shared_ptr<While> stmt) = 0;
        T virtual visit_fun_stmt(std::shared_ptr<Function> stmt) = 0;
        T virtual visit_return_stmt(Return &stmt) = 0;
    };
}