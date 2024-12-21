#pragma once
#include <string>
#include <variant>

class Token
{
public:

    using Literal = std::variant<std::monostate, double, std::string>;

    enum class TokenType
    {
        // single character tokens
        LEFT_PAREN,
        RIGHT_PAREN,
        LEFT_BRACE,
        RIGHT_BRACE,
        COMMA,
        DOT,
        MINUS,
        PLUS,
        SEMICOLON,
        SLASH,
        STAR,
        BANG,

        // one or two character tokens
        BANG_EQUAL,
        EQUAL,
        EQUAL_EQUAL,
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL,

        // literals
        IDENTIFIER,
        STRING,
        NUMBER,

        // keywords
        AND,
        CLASS,
        ELSE,
        FALSE,
        FUN,
        FOR,
        IF,
        NIL,
        OR,
        PRINT,
        RETURN,
        SUPER,
        THIS,
        TRUE,
        VAR,
        WHILE,

        END_OF_FILE,
    };

    Token(TokenType type, std::string lexeme, Literal literal, int line) : type(type), lexeme(lexeme), literal(literal), line(line)
    {
    }

    [[nodiscard]] std::string to_string() const
    {
        return "[" + name(type) + ", " + lexeme + "]";
    }

    [[nodiscard]] TokenType get_type() const { return type; }

    [[nodiscard]] std::string get_lexeme() const { return lexeme; }

    [[nodiscard]] Literal get_literal() const { return literal; }

private:
    std::string name(TokenType type) const
    {
        switch (type)
        {
        case TokenType::LEFT_PAREN:
            return "LEFT_PAREN";
            break;
        case TokenType::RIGHT_PAREN:
            return "RIGHT_PAREN";
            break;
        case TokenType::LEFT_BRACE:
            return "LEFT_BRACE";
            break;
        case TokenType::RIGHT_BRACE:
            return "RIGHT_BRACE";
            break;
        case TokenType::COMMA:
            return "COMMA";
            break;
        case TokenType::DOT:
            return "DOT";
            break;
        case TokenType::MINUS:
            return "MINUS";
            break;
        case TokenType::PLUS:
            return "PLUS";
            break;
        case TokenType::SEMICOLON:
            return "SEMICOLON";
            break;
        case TokenType::SLASH:
            return "SLASH";
            break;
        case TokenType::STAR:
            return "STAR";
            break;
        case TokenType::BANG:
            return "BANG";
            break;
        case TokenType::BANG_EQUAL:
            return "BANG_EQUAL";
            break;
        case TokenType::EQUAL:
            return "EQUAL";
            break;
        case TokenType::EQUAL_EQUAL:
            return "EQUAL_EQUAL";
            break;
        case TokenType::GREATER:
            return "GREATER";
            break;
        case TokenType::GREATER_EQUAL:
            return "GREATER_EQUAL";
            break;
        case TokenType::LESS:
            return "LESS";
            break;
        case TokenType::LESS_EQUAL:
            return "LESS_EQUAL";
            break;
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
            break;
        case TokenType::STRING:
            return "STRING";
            break;
        case TokenType::NUMBER:
            return "NUMBER";
            break;
        case TokenType::AND:
            return "AND";
            break;
        case TokenType::CLASS:
            return "CLASS";
            break;
        case TokenType::ELSE:
            return "ELSE";
            break;
        case TokenType::FALSE:
            return "FALSE";
            break;
        case TokenType::FUN:
            return "FUN";
            break;
        case TokenType::FOR:
            return "FOR";
            break;
        case TokenType::IF:
            return "IF";
            break;
        case TokenType::NIL:
            return "NIL";
            break;
        case TokenType::OR:
            return "OR";
            break;
        case TokenType::PRINT:
            return "PRINT";
            break;
        case TokenType::RETURN:
            return "RETURN";
            break;
        case TokenType::SUPER:
            return "SUPER";
            break;
        case TokenType::THIS:
            return "THIS";
            break;
        case TokenType::TRUE:
            return "TRUE";
            break;
        case TokenType::VAR:
            return "VAR";
            break;
        case TokenType::WHILE:
            return "WHILE";
            break;
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
            break;
        default:
            return "UNKNOWN";
            break;
        };
    }

private:
    TokenType type;
    std::string lexeme;
    Literal literal;
    int line;
};