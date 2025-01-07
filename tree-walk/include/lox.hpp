#pragma once

#include <fstream>
#include <interpreter.hpp>
#include <iostream>
#include <scanner.hpp>
#include <sstream>
#include <sysexits.h>

class Lox
{
public:
  Lox() = default;

  void run(const std::string &src, bool repl);

  void run_prompt()
  {
    std::string line;
    std::cout << "> ";
    while(std::getline(std::cin, line)) {
      run(line, true);
      had_error = false;
      std::cout << "> ";
    }
  }

  void run_file(const std::string &path)
  {
    std::ifstream fs(path);
    if(!fs.is_open()) {
      std::cout << "Could not open file" << std::endl;
    }

    std::stringstream buffer;
    buffer << fs.rdbuf();
    run(buffer.str(), false);

    if(had_error) {
      exit(EX_DATAERR);
    }

    if(had_runtime_error) {
      exit(EX_SOFTWARE);
    }
  }

  static void error(int line, const std::string &message) { report(line, "", message); }

  static void error(Token token, const std::string &message)
  {
    if(token.get_type() == Token::TokenType::END_OF_FILE) {
      Lox::report(token.get_line(), " at end", message);
    }
    else {
      Lox::report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
    }
  }

  static void report(int line, const std::string &where, const std::string &message)
  {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
    had_error = true;
  }

  static void runtime_error(const Interpreter::RuntimeError &error)
  {
    std::cerr << error.what() << std::endl
              << "[line " << error.token.get_line() << "]" << std::endl;
    had_runtime_error = true;
  }

private:
  Interpreter interpreter;
  static bool had_runtime_error;
  static bool had_error;
};