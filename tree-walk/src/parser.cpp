#include <parser.hpp>
#include <lox.hpp>

std::unique_ptr<expr::ExprBase> Parser::expression()
{
    return assignment();
}

std::unique_ptr<expr::ExprBase> Parser::equality()
{
    auto expr = comparison();
    while (match(Token::TokenType::BANG_EQUAL, Token::TokenType::EQUAL_EQUAL))
    {
        auto op = previous();
        auto right = comparison();
        expr = std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<expr::ExprBase> Parser::comparison()
{
    auto expr = term();
    while (match(Token::TokenType::GREATER, Token::TokenType::GREATER_EQUAL, Token::TokenType::LESS, Token::TokenType::LESS_EQUAL))
    {
        auto op = previous();
        auto right = term();
        expr = std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<expr::ExprBase> Parser::term()
{
    auto expr = factor();
    while (match(Token::TokenType::MINUS, Token::TokenType::PLUS))
    {
        auto op = previous();
        auto right = factor();
        expr = std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<expr::ExprBase> Parser::factor()
{
    auto expr = unary();
    while (match(Token::TokenType::SLASH, Token::TokenType::STAR))
    {
        auto op = previous();
        auto right = unary();
        expr = std::make_unique<expr::Binary>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<expr::ExprBase> Parser::unary()
{
    if (match(Token::TokenType::BANG, Token::TokenType::MINUS))
    {
        auto op = previous();
        auto right = unary();
        return std::make_unique<expr::Unary>(op, std::move(right));
    }
    return primary();
}

std::unique_ptr<expr::ExprBase> Parser::primary()
{
    if (match(Token::TokenType::FALSE))
    {
        return std::make_unique<expr::Literal>(Token::Literal{false});
    }
    else if (match(Token::TokenType::TRUE))
    {
        return std::make_unique<expr::Literal>(Token::Literal{true});
    }
    else if (match(Token::TokenType::NIL))
    {
        return std::make_unique<expr::Literal>(Token::Literal{});
    }
    else if (match(Token::TokenType::NUMBER, Token::TokenType::STRING))
    {
        return std::make_unique<expr::Literal>(previous().get_literal());
    }
    else if (match(Token::TokenType::IDENTIFIER))
    {
        return std::make_unique<expr::Variable>(previous());
    }
    else if (match(Token::TokenType::LEFT_PAREN))
    {
        auto expr = expression();
        consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return std::make_unique<expr::Grouping>(std::move(expr));
    }
    throw error(peek(), "Expect expression.");
}

std::unique_ptr<expr::ExprBase> Parser::assignment()
{
    // trick: parse the left side as a single expression so that we automatically stop
    // at the first '='
    auto expr = logical_or();

    if (match(Token::TokenType::EQUAL))
    {
        auto equals = previous();
        auto value = assignment(); // recursively parse the right side

        if (typeid(*expr) == typeid(expr::Variable))
        {
            auto &name = dynamic_cast<expr::Variable &>(*expr);
            return std::make_unique<expr::Assignment>(name.token, std::move(value));
        }
        error(equals, "Invalid assignment target.");
    }
    return expr;
}

std::unique_ptr<expr::ExprBase> Parser::logical_or()
{
    auto expr = logical_and();
    while (match(Token::TokenType::OR))
    {
        auto op = previous();
        auto right = logical_and();
        expr = std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
    }
    return expr;
}
std::unique_ptr<expr::ExprBase> Parser::logical_and()
{
    auto expr = equality();
    while (match(Token::TokenType::AND))
    {
        auto op = previous();
        auto right = equality();
        expr = std::make_unique<expr::Logical>(std::move(expr), op, std::move(right));
    }
    return expr;
}

Parser::ParseError Parser::error(Token token, const std::string &message)
{
    Lox::error(token, message);
    return ParseError();
}

Token Parser::consume(Token::TokenType type, const std::string &message)
{
    if (check(type))
    {
        return advance();
    }
    else
    {
        throw error(peek(), message);
    }
}

std::unique_ptr<stmt::StmtBase> Parser::statement()
{
    if (match(Token::TokenType::PRINT))
    {
        return print_statement();
    }
    else if (match(Token::TokenType::WHILE))
    {
        return while_statement();
    }
    else if (match(Token::TokenType::FOR))
    {
        return for_statement();
    }
    else if (match(Token::TokenType::LEFT_BRACE))
    {
        // why not just block()?
        // because block() is used to parse function blocks and we don't want the result to
        // be embedded in a stmt::Block
        return std::make_unique<stmt::Block>(block());
    }
    else if (match(Token::TokenType::IF))
    {
        return if_statement();
    }
    return expr_statement();
}

std::unique_ptr<stmt::StmtBase> Parser::print_statement()
{
    auto expr = expression();
    consume(Token::TokenType::SEMICOLON, "Expect ';' after value.");
    return std::make_unique<stmt::Print>(std::move(expr));
}

std::unique_ptr<stmt::StmtBase> Parser::expr_statement()
{
    auto expr = expression();
    if (!repl || peek().get_type() == Token::TokenType::SEMICOLON)
    {
        consume(Token::TokenType::SEMICOLON, "Expect ';' after expression.");
    }
    return std::make_unique<stmt::Expression>(std::move(expr));
}

std::unique_ptr<stmt::StmtBase> Parser::declaration()
{
    try
    {
        if (match(Token::TokenType::VAR))
        {
            return var_declaration();
        }
        return statement();
    }
    catch (const ParseError &)
    {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<stmt::StmtBase> Parser::var_declaration()
{

    Token name = consume(Token::TokenType::IDENTIFIER, "Expect variable name.");
    std::unique_ptr<expr::ExprBase> initializer{};

    if (match(Token::TokenType::EQUAL))
    {
        initializer = expression();
    }

    consume(Token::TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<stmt::VariableDecl>(name, std::move(initializer));
}

std::vector<std::unique_ptr<stmt::StmtBase>> Parser::block()
{
    std::vector<std::unique_ptr<stmt::StmtBase>> statements;
    while (!check(Token::TokenType::RIGHT_BRACE) && !is_at_end())
    {
        statements.push_back(declaration());
    }
    consume(Token::TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return statements;
}

std::unique_ptr<stmt::StmtBase> Parser::if_statement()
{
    consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    auto then_stm = statement();
    std::unique_ptr<stmt::StmtBase> else_stm = nullptr;
    if (match(Token::TokenType::ELSE))
    {
        else_stm = statement();
    }
    return std::make_unique<stmt::If>(std::move(condition), std::move(then_stm), std::move(else_stm));
}

std::unique_ptr<stmt::StmtBase> Parser::while_statement()
{
    consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    auto condition = expression();
    consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after condition.");
    auto body = statement();
    return std::make_unique<stmt::While>(std::move(condition), std::move(body));
}

std::unique_ptr<stmt::StmtBase> Parser::for_statement()
{
    consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    std::unique_ptr<stmt::StmtBase> initializer;
    if (match(Token::TokenType::SEMICOLON))
    {
        initializer = nullptr;
    }
    else if (match(Token::TokenType::VAR))
    {
        initializer = var_declaration();
    }
    else
    {
        initializer = expr_statement();
    }
    // first ; parsed
    std::unique_ptr<expr::ExprBase> condition = nullptr;
    if (!check(Token::TokenType::SEMICOLON))
    {
        condition = expression();
    }
    consume(Token::TokenType::SEMICOLON, "Expect ';' after loop condition.");

    std::unique_ptr<expr::ExprBase> increment = nullptr;
    if (!check(Token::TokenType::RIGHT_PAREN))
    {
        increment = expression();
    }
    consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

    auto body = statement();

    // desugaring the for loop into a while loop

    if (increment)
    {
        auto statements = std::vector<std::unique_ptr<stmt::StmtBase>>();
        statements.emplace_back(std::move(body));
        // TODO: in repl this gets printed
        statements.push_back(std::make_unique<stmt::Expression>(std::move(increment)));
        body = std::make_unique<stmt::Block>(std::move(statements));
    }
    if (!condition)
    {
        condition = std::make_unique<expr::Literal>(Token::Literal{true});
    }
    body = std::make_unique<stmt::While>(std::move(condition), std::move(body));

    if (initializer)
    {
        auto statements = std::vector<std::unique_ptr<stmt::StmtBase>>();
        statements.emplace_back(std::move(initializer));
        statements.emplace_back(std::move(body));
        body = std::make_unique<stmt::Block>(std::move(statements));
    }
    return body;
}

void Parser::synchronize()
{
    while (!is_at_end())
    {
        switch (peek().get_type())
        {
        case Token::TokenType::CLASS:
        case Token::TokenType::FUN:
        case Token::TokenType::VAR:
        case Token::TokenType::FOR:
        case Token::TokenType::IF:
        case Token::TokenType::WHILE:
        case Token::TokenType::PRINT:
        case Token::TokenType::RETURN:
            return;
        }

        advance();
        if (previous().get_type() == Token::TokenType::SEMICOLON)
            return;
    }
}