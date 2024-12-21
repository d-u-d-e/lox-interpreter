#include <iostream>
#include <sysexits.h>
#include <lox.hpp>

int main(int argc, char *argv[])
{
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