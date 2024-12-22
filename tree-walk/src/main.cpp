#include <iostream>
#include <sysexits.h>
#include <lox.hpp>

#include <ast/ast_visitor.hpp>
#include <token.hpp>

int main(int argc, char *argv[])
{
    auto expr = std::make_unique<expr::Binary>(
        std::make_unique<expr::Unary>(
            Token(Token::TokenType::MINUS, "-", nullptr, 1),
            std::make_unique<expr::Literal>(Token::Literal{123})
        ),
        Token(Token::TokenType::STAR, "*", nullptr, 1),
        std::make_unique<expr::Grouping>(
            std::make_unique<expr::Literal>(Token::Literal{45.67})
        )
    );

    ASTVisitor visitor;
    std::cout << visitor.print(*expr) << std::endl;
    return 0;

    if (argc > 2)
    {
        std::cout << "Usage: cpplox [script]" << std::endl;
        return EX_USAGE;
    }

    Lox lox;

    if (argc == 2)
    {
        std::cout << "Running " << argv[1] << std::endl;
        lox.run_file(argv[1]);
    }
    else
    {
        std::cout << "Starting REPL" << std::endl;
        lox.run_prompt();
    }
    return 0;
}