#pragma once
#include <token.hpp>
#include <vector>
#include <ast/stmt.hpp>

class Parser
{
public:
    class ParseError : public std::exception
    {
    };

    Parser(const std::vector<Token> &tokens) : tokens(tokens) {}

    std::vector<std::unique_ptr<stmt::StmtBase>> parse()
    {
        std::vector<std::unique_ptr<stmt::StmtBase>> statements;

        while (!is_at_end())
        {
            statements.push_back(declaration());
        }

        return statements;
    }  

private:
    ParseError error(Token token, const std::string &message);
    Token consume(Token::TokenType type, const std::string &message);
    void synchronize();

    std::unique_ptr<expr::ExprBase> expression();
    std::unique_ptr<expr::ExprBase> equality();
    std::unique_ptr<expr::ExprBase> comparison();
    std::unique_ptr<expr::ExprBase> term();
    std::unique_ptr<expr::ExprBase> factor();
    std::unique_ptr<expr::ExprBase> unary();
    std::unique_ptr<expr::ExprBase> primary();
    std::unique_ptr<expr::ExprBase> assignment();

    std::unique_ptr<stmt::StmtBase> statement();
    std::unique_ptr<stmt::StmtBase> print_statement();
    std::unique_ptr<stmt::StmtBase> expr_statement();
    std::unique_ptr<stmt::StmtBase> declaration();
    std::unique_ptr<stmt::StmtBase> var_declaration();
    std::vector<std::unique_ptr<stmt::StmtBase>> block();

    bool is_at_end()
    {
        return peek().get_type() == Token::TokenType::END_OF_FILE;
    }

    Token peek()
    {
        return tokens[current];
    }
    Token previous() { return tokens[current - 1]; }

    bool check(Token::TokenType type)
    {
        if (is_at_end())
        {
            return false;
        }
        return peek().get_type() == type;
    }

    Token advance()
    {
        if (!is_at_end())
        {
            current++;
        }
        return previous();
    }

    template <typename... Args>
    bool match(Args... types)
    {
        for (auto type : {types...})
        {
            if (check(type))
            {
                advance();
                return true;
            }
        }
        return false;
    }

private:
    size_t current{0};
    std::vector<Token> tokens;
};