#include <ast/expr.hpp>

class ASTVisitor : public expr::Visitor<std::string>
{
public:
    std::string visit_binary(const expr::Binary &expr) override
    {
        return parenthesize(expr.token.get_lexeme(), *expr.left.get(), *expr.right.get());
    }
    std::string visit_grouping(const expr::Grouping &expr) override
    {
        return parenthesize("group", *expr.expr.get());
    }
    std::string visit_literal(const expr::Literal &expr) override
    {
        if (std::holds_alternative<std::string>(expr.value))
        {
            return std::get<std::string>(expr.value);
        }
        else if (std::holds_alternative<std::nullptr_t>(expr.value))
        {
            return "nil";
        }
        else
        {
            return std::to_string(std::get<double>(expr.value));
        }
    }
    std::string visit_unary(const expr::Unary &expr) override
    {
        return parenthesize(expr.token.get_lexeme(), *expr.right.get());
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