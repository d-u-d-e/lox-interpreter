#include <ast/ast_visitor.hpp>
#include <interpreter.hpp>
#include <lox.hpp>
#include <parser.hpp>
#include <resolver.hpp>

bool Lox::had_error{false};
bool Lox::had_runtime_error{false};

void Lox::run(const std::string &src, bool repl)

{
  Scanner scanner(src);
  std::vector<Token> tokens = scanner.scan_tokens();

  if(had_error) {
    return;
  }

  Parser parser(tokens, repl);
  auto statements = parser.parse();

  if(had_error) {
    return;
  }

  Resolver resolver(interpreter);
  resolver.resolve(statements);

  if(had_error) {
    return;
  }

  interpreter.interpret(statements, repl);
}
