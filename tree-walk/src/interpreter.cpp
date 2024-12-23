#include <interpreter.hpp>
#include <lox.hpp>

expr::value Interpreter::visit_binary_expr(const expr::Binary &expr)
{
    expr::value left = evaluate(*expr.left);
    expr::value right = evaluate(*expr.right);

    switch (expr.token.get_type())
    {
    case Token::TokenType::MINUS:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) - std::get<double>(right);

    case Token::TokenType::STAR:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) * std::get<double>(right);

    case Token::TokenType::SLASH:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) / std::get<double>(right);

    case Token::TokenType::PLUS:
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
        {
            return std::get<double>(left) + std::get<double>(right);
        }
        else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
        {
            return std::get<std::string>(left) + std::get<std::string>(right);
        }

        throw RuntimeError(expr.token, "Operands must be two numbers or two strings.");

    case Token::TokenType::GREATER:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) > std::get<double>(right);

    case Token::TokenType::GREATER_EQUAL:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) >= std::get<double>(right);

    case Token::TokenType::LESS:
        check_number_operands(expr.token, left, right);
        return std::get<double>(left) < std::get<double>(right);

    case Token::TokenType::EQUAL_EQUAL:
        return is_equal(left, right);

    case Token::TokenType::BANG_EQUAL:
        return !is_equal(left, right);
    }

    // unreachable
    throw std::runtime_error("interpreter error");
}
expr::value Interpreter::visit_grouping_expr(const expr::Grouping &expr)
{
    return evaluate(*expr.expr);
}
expr::value Interpreter::visit_literal_expr(const expr::Literal &expr)
{
    return expr.value;
}

bool Interpreter::is_truthy(const expr::value &value)
{
    if (std::holds_alternative<std::nullptr_t>(value))
    {
        return false;
    }
    else if (std::holds_alternative<bool>(value))
    {
        return std::get<bool>(value);
    }
    return true;
}

bool Interpreter::is_equal(const expr::value &left, expr::value &right)
{
    if (std::holds_alternative<std::nullptr_t>(left) && std::holds_alternative<std::nullptr_t>(right))
    {
        return true;
    }
    else if (std::holds_alternative<std::nullptr_t>(left))
    {
        return false;
    }

    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
    {
        return std::get<double>(left) == std::get<double>(right);
    }
    else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
    {
        return std::get<std::string>(left) == std::get<std::string>(right);
    }
    if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right))
    {
        return std::get<bool>(left) == std::get<bool>(right);
    }
    return false;
}

expr::value Interpreter::visit_unary_expr(const expr::Unary &expr)
{
    expr::value right = evaluate(*expr.right);

    switch (expr.token.get_type())
    {
    case Token::TokenType::MINUS:
        check_number_operand(expr.token, right);
        return -std::get<double>(right);

    case Token::TokenType::BANG:
        return !is_truthy(right);

    default:
        // unreachable
        return nullptr;
    }
}

void Interpreter::check_number_operand(const Token &op, const expr::value &operand)
{
    if (std::holds_alternative<double>(operand))
    {
        return;
    }
    throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token &op, const expr::value &left, const expr::value &right)
{
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
    {
        return;
    }
    throw RuntimeError(op, "Operands must be numbers.");
}

std::string Interpreter::stringify(const expr::value &value)
{
    if (std::holds_alternative<std::nullptr_t>(value))
    {
        return "nil";
    }
    else if (std::holds_alternative<double>(value))
    {
        return std::to_string(std::get<double>(value));
    }
    else if (std::holds_alternative<std::string>(value))
    {
        return std::get<std::string>(value);
    }
    else if (std::holds_alternative<bool>(value))
    {
        return std::get<bool>(value) ? "true" : "false";
    }
    return "";
}

void Interpreter::visit_print_stmt(const stmt::Print &stmt)
{
    auto value = evaluate(*stmt.ex.get());
    std::cout << stringify(value) << std::endl;
}

void Interpreter::visit_expr_stmt(const stmt::Expression &stmt)
{
    // discard value
    evaluate(*stmt.ex.get());
}

expr::value Interpreter::visit_variable_expr(const expr::Variable &expr)
{
    return env.get(expr.token);
}

void Interpreter::visit_vardecl_stmt(const stmt::VariableDecl &stmt)
{
    // by default, if a variable declaration has no initializer, the value is nil
    expr::value value{nullptr};
    if (stmt.initializer != nullptr)
    {
        value = evaluate(*stmt.initializer);
    }

    env.define(stmt.token.get_lexeme(), value);
}

void Interpreter::interpret(const std::vector<std::unique_ptr<stmt::StmtBase>> &stms)
{
    try
    {
        for (const auto &stm : stms)
        {
            execute(*stm);
        }
    }
    catch (const RuntimeError &error)
    {
        Lox::runtime_error(error);
    }
}
