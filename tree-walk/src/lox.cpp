#include <lox.hpp>
#include <parser.hpp>
#include <ast/ast_visitor.hpp>
#include <interpreter.hpp>

bool Lox::had_error{false};
bool Lox::had_runtime_error{false};

void Lox::run(const std::string &src)

{
    Scanner scanner(src);
    std::vector<Token> tokens = scanner.scan_tokens();

    if (had_error)
    {
        return;
    }

    Parser parser(tokens);
    auto statements = parser.parse();

    if (had_error)
    {
        return;
    }

    Interpreter interpreter;
    interpreter.interpret(statements);

    // ASTVisitor visitor;
    // std::cout << visitor.print(*expr) << std::endl;
}
