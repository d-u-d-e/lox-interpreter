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

    Parser(const std::vector<Token> &tokens, bool repl = false) : tokens(tokens), repl(repl) {}

    std::vector<std::shared_ptr<stmt::StmtBase>> parse()
    {
        std::vector<std::shared_ptr<stmt::StmtBase>> statements;

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

    std::shared_ptr<expr::ExprBase> equality();
    std::shared_ptr<expr::ExprBase> comparison();
    std::shared_ptr<expr::ExprBase> term();
    std::shared_ptr<expr::ExprBase> factor();
    std::shared_ptr<expr::ExprBase> unary();
    std::shared_ptr<expr::ExprBase> primary();
    std::shared_ptr<expr::ExprBase> assignment();
    std::shared_ptr<expr::ExprBase> logical_or();
    std::shared_ptr<expr::ExprBase> logical_and();
    std::shared_ptr<expr::ExprBase> call();
    std::shared_ptr<expr::ExprBase> finish_call(std::shared_ptr<expr::ExprBase> callee);
    std::shared_ptr<expr::ExprBase> expression();

    std::shared_ptr<stmt::StmtBase> print_statement();
    std::shared_ptr<stmt::StmtBase> expr_statement();
    std::shared_ptr<stmt::StmtBase> declaration();
    std::shared_ptr<stmt::StmtBase> var_declaration();
    std::shared_ptr<stmt::StmtBase> statement();
    std::vector<std::shared_ptr<stmt::StmtBase>> block();
    std::shared_ptr<stmt::StmtBase> if_statement();
    std::shared_ptr<stmt::StmtBase> while_statement();
    std::shared_ptr<stmt::StmtBase> for_statement();
    std::shared_ptr<stmt::StmtBase> function(std::string kind);
    std::shared_ptr<stmt::StmtBase> return_statement();

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
    bool repl;
    size_t current{0};
    std::vector<Token> tokens;
};