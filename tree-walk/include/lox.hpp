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

    void run(const std::string &src)
    {
        Scanner scanner(src);
        std::vector<Token> tokens = scanner.scan_tokens();

        if (had_error)
        {
            exit(EX_DATAERR);
        }

        // TODO
        for (auto &token : tokens)
        {
            std::cout << token.to_string() << std::endl;
        }
    }

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

    static void report(int line, const std::string &where, const std::string &message)
    {
        std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
        had_error = true;
    }

private:
    static bool had_error;
};