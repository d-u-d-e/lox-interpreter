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
        // shared ptr required to resolve a variable by ptr value
        T virtual visit_variable_expr(const std::shared_ptr<const Variable> & expr) = 0;
        T virtual visit_assignment_expr(const std::shared_ptr<const expr::Assignment> & expr) = 0;
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
        T virtual visit_print_stmt(const Print &stmt) = 0;
        T virtual visit_expr_stmt(const Expression &stmt) = 0;
        T virtual visit_vardecl_stmt(const VariableDecl &stmt) = 0;
        T virtual visit_block_stmt(const Block &stmt) = 0;
        T virtual visit_if_stmt(const If &stmt) = 0;
        T virtual visit_while_stmt(const std::shared_ptr<const While> & stmt) = 0;
        T virtual visit_fun_stmt(const std::shared_ptr<const Function> & stmt) = 0;
        T virtual visit_return_stmt(const Return &stmt) = 0;
    };
}