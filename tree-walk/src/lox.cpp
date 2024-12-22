#include <lox.hpp>
#include <parser.hpp>
#include <ast/ast_visitor.hpp>

bool Lox::had_error{false};

void Lox::run(const std::string &src)

{
    Scanner scanner(src);
    std::vector<Token> tokens = scanner.scan_tokens();

    if (had_error)
    {
        return;
    }

    Parser parser(tokens);
    auto expr = parser.parse();

    if (had_error)
    {
        return;
    }

    ASTVisitor visitor;
    std::cout << visitor.print(*expr) << std::endl;
}
