#pragma once

#include <string>
#include <iostream>
#include <scanner.hpp>
#include <fstream>
#include <sysexits.h>
#include <sstream>

class Lox
{
public:
    Lox() = default;

    void run(const std::string &src);

    void run_prompt()
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
            run(line);
            had_error = false;
        }
    }

    void run_file(const std::string &path)
    {
        std::ifstream fs(path);
        if (!fs.is_open())
        {
            std::cout << "Could not open file" << std::endl;
        }

        std::stringstream buffer;
        buffer << fs.rdbuf();
        run(buffer.str());
    }

    static void error(int line, const std::string &message)
    {
        report(line, "", message);
    }

    static void error(Token token, const std::string &message)
    {
        if (token.get_type() == Token::TokenType::END_OF_FILE)
        {
            Lox::report(token.get_line(), " at end", message);
        }
        else
        {
            Lox::report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
        }
    }

    static void report(int line, const std::string &where, const std::string &message)
    {
        std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
        had_error = true;
    }

private:
    static bool had_error;
};