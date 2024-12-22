#include <ast/expr.hpp>

class ASTVisitor : public expr::Visitor<std::string>
{
public:
    std::string visitBinary(const expr::Binary &expr) override
    {
        return parenthesize(expr.token.get_lexeme(), *expr.left.get(), *expr.right.get());
    }
    std::string visitGrouping(const expr::Grouping &expr) override
    {
        return parenthesize("group", *expr.expr.get());
    }
    std::string visitLiteral(const expr::Literal &expr) override
    {
        if (std::holds_alternative<std::string>(expr.literal))
        {
            return std::get<std::string>(expr.literal);
        }
        else if (std::holds_alternative<std::nullptr_t>(expr.literal))
        {
            return "nil";
        }
        else if (std::holds_alternative<int>(expr.literal))
        {
            return std::to_string(std::get<int>(expr.literal));
        }
        else
        {
            return std::to_string(std::get<double>(expr.literal));
        }
    }
    std::string visitUnary(const expr::Unary &expr) override
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