#pragma once

#include <string>
#include <iostream>
#include <scanner.hpp>
#include <fstream>
#include <sysexits.h>

class Lox
{
public:
    Lox() = default;

    void run(const std::string &src)
    {
        Scanner scanner(src);
        std::vector<Token> tokens = scanner.scan_tokens();

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
        std::fstream fs;
        fs.open(path.c_str(), std::ios::in);
        if (!fs.is_open())
        {
            std::cout << "Could not open file" << std::endl;
        }
        std::string src;
        std::string line;
        while (std::getline(fs, line))
        {
            src += line;
        }
        run(src);

        if (had_error)
        {
            exit(EX_DATAERR);
        }
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