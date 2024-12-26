#include <ast/expr.hpp>

class ASTVisitor : public expr::Visitor<std::string>
{
public:
    std::string visit_binary_expr(const expr::Binary &expr) override
    {
        return parenthesize(expr.op.get_lexeme(), *expr.left.get(), *expr.right.get());
    }
    std::string visit_grouping_expr(const expr::Grouping &expr) override
    {
        return parenthesize("group", *expr.expr.get());
    }
    std::string visit_literal_expr(const expr::Literal &expr) override
    {
        if (expr.value.is_string())
        {
            return std::get<std::string>(expr.value.v);
        }
        else if (expr.value.is_double())
        {
            return std::to_string(std::get<double>(expr.value.v));
        }
        else
        {
            return "nil";
        }
    }
    std::string visit_unary_expr(const expr::Unary &expr) override
    {
        return parenthesize(expr.op.get_lexeme(), *expr.right.get());
    }

    std::string print(const expr::ExprBase &expr)
    {
        return expr.accept(*this);
    }

private:
    template <typename... Args>
    std::string parenthesize(const std::string &name, Args &&...expr)
    {
        std::string result = "(" + name;

        ([&]
         { result += " " + expr.accept(*this); }(), ...);

        result += ")";
        return result;
    }
};