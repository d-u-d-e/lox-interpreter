#include <scanner.hpp>
#include <lox.hpp>

const std::unordered_map<std::string, Token::TokenType> Scanner::keywords{
    {"and", Token::TokenType::AND},
    {"class", Token::TokenType::CLASS},
    {"else", Token::TokenType::ELSE},
    {"false", Token::TokenType::FALSE},
    {"for", Token::TokenType::FOR},
    {"fun", Token::TokenType::FUN},
    {"if", Token::TokenType::IF},
    {"nil", Token::TokenType::NIL},
    {"or", Token::TokenType::OR},
    {"print", Token::TokenType::PRINT},
    {"return", Token::TokenType::RETURN},
    {"super", Token::TokenType::SUPER},
    {"this", Token::TokenType::THIS},
    {"true", Token::TokenType::TRUE},
    {"var", Token::TokenType::VAR},
    {"while", Token::TokenType::WHILE}};

std::vector<Token> Scanner::scan_tokens()
{
    while (!is_at_end())
    {
        start = current;
        scan_token();
    }

    tokens.push_back(Token(Token::TokenType::END_OF_FILE, "", "", line));
    return tokens;
}

void Scanner::scan_token()
{
    char c = advance();
    switch (c)
    {
    case '(':
        add_token(Token::TokenType::LEFT_PAREN);
        break;
    case ')':
        add_token(Token::TokenType::RIGHT_PAREN);
        break;
    case '{':
        add_token(Token::TokenType::LEFT_BRACE);
        break;
    case '}':
        add_token(Token::TokenType::RIGHT_BRACE);
        break;
    case ',':
        add_token(Token::TokenType::COMMA);
        break;
    case '.':
        add_token(Token::TokenType::DOT);
        break;
    case '-':
        add_token(Token::TokenType::MINUS);
        break;
    case '+':
        add_token(Token::TokenType::PLUS);
        break;
    case ';':
        add_token(Token::TokenType::SEMICOLON);
        break;
    case '*':
        add_token(Token::TokenType::STAR);
        break;
    case '!':
        add_token(match('=') ? Token::TokenType::BANG_EQUAL : Token::TokenType::BANG);
        break;
    case '=':
        add_token(match('=') ? Token::TokenType::EQUAL_EQUAL : Token::TokenType::EQUAL);
        break;
    case '<':
        add_token(match('=') ? Token::TokenType::LESS_EQUAL : Token::TokenType::LESS);
        break;
    case '>':
        add_token(match('=') ? Token::TokenType::GREATER_EQUAL : Token::TokenType::GREATER);
        break;
    case '/':
        if (match('/'))
        {
            // A comment goes until the end of the line.
            while (peek() != '\n' && !is_at_end())
            {
                advance();
            }
        }
        else
        {
            add_token(Token::TokenType::SLASH);
        }
        break;
    case ' ':
    case '\r':
    case '\t':
        // Ignore whitespace.
        break;
    case '\n':
        line++;
        break;
    case '"':
        string();
        break;
    default:
        if (std::isdigit(c))
        {
            number();
        }

        else if (std::isalpha(c))
        {
            identifier();
        }
        else
        {
            Lox::error(line, "Unexpected character.");
            break;
        }
    }
}

char Scanner::advance()
{
    return source[current++];
}

void Scanner::number()
{
    while (std::isdigit(peek()))
    {
        advance();
    }
    // Look for a fractional part.
    if (peek() == '.' && std::isdigit(peek_next()))
    {
        // Consume the "."
        advance();
        while (std::isdigit(peek()))
        {
            advance();
        }
    }
    add_token(Token::TokenType::NUMBER, std::stod(source.substr(start, current - start)));
}

void Scanner::identifier()
{
    while (std::isalnum(peek()))
    {
        advance();
    }

    std::string text = source.substr(start, current - start);
    Token::TokenType type{};

    if (keywords.find(text) != keywords.end())
    {
        type = keywords.at(text);
    }
    else
    {
        type = Token::TokenType::IDENTIFIER;
    }
    add_token(type);
}

void Scanner::string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n')
        {
            line++;
        }
        advance();
    }
    if (is_at_end())
    {
        Lox::error(line, "Unterminated string.");
        return;
    }
    // The closing quote
    advance();
    // Trim the surrounding quotes.
    std::string value = source.substr(start + 1, current - start - 2);
    add_token(Token::TokenType::STRING, value);
}