#pragma once
#include <token.hpp>
#include <vector>
#include <unordered_map>

class Scanner
{
public:
    Scanner(std::string source) : source(source) {}
    [[nodiscard]] std::vector<Token> scan_tokens();

private:
    const static std::unordered_map<std::string, Token::TokenType> keywords;

    void scan_token();
    bool is_at_end() { return current >= source.size(); }
    char advance();
    bool match(char expected)
    {
        if (is_at_end())
        {
            return false;
        }
        else if (source[current] == expected)
        {
            current++;
            return true;
        }
        return false;
    }

    char peek() { return is_at_end() ? '\0' : source[current]; }

    char peek_next() { 
        if (current + 1 >= source.size())
        {
            return '\0';
        }
        return source[current + 1];
    }

    void add_token(Token::TokenType type)
    {
        add_token(type, nullptr);
    }

    void add_token(Token::TokenType type, Token::Literal literal)
    {
        tokens.push_back(Token(type, source.substr(start, current - start), literal, line));
    }

    void string();
    void number();
    void identifier();

private:
    int line{1};
    std::vector<Token> tokens;
    size_t start{0};
    size_t current{0};
    std::string source;
};