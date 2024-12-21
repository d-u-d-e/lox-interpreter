#include <iostream>
#include <sysexits.h>
#include <filesystem>
#include <fstream>

void run(std::string src)
{
    std::cout << src << std::endl;
}

void run_prompt()
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        run(line);
    }
}

void run_file(std::filesystem::path path)
{
    std::fstream fs;
    fs.open(path.c_str(), std::ios::in);
    if (!fs.is_open())
    {
        std::cout << "Could not open file" << std::endl;
    }
    std::string line;
    while (std::getline(fs, line))
    {
        run(line);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        std::cout << "Usage: cpplox [script]" << std::endl;
        return EX_USAGE;
    }
    else if (argc == 2)
    {
        std::cout << "Running " << argv[1] << std::endl;
        run_file(argv[1]);
    }
    else
    {
        std::cout << "Starting REPL" << std::endl;
        run_prompt();
    }
    return 0;
}