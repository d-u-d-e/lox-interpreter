#include <common.h>
#include <scanner.h>
#include <string.h>

typedef struct {
  const char *start;
  const char *current;
  int line;
} scanner_t;

scanner_t g_scanner;

void init_scanner(const char *source)
{
  g_scanner.start = source;
  g_scanner.current = source;
  g_scanner.line = 1;
}

static bool is_alpha(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_at_end() { return *g_scanner.current == '\0'; }

static char advance()
{
  g_scanner.current++;
  return g_scanner.current[-1];
}

static char peek() { return *g_scanner.current; }

static char peek_next()
{
  if(is_at_end()) {
    return '\0';
  }
  return g_scanner.current[1];
}

static bool match(char expected)
{
  if(is_at_end()) {
    return false;
  }
  else if(*g_scanner.current == expected) {
    g_scanner.current++;
    return true;
  }
  return false;
}

static token_t make_token(token_type_t type)
{
  token_t token;
  token.type = type;
  token.start = g_scanner.start;
  token.length = (int)(g_scanner.current - g_scanner.start);
  token.line = g_scanner.line;
  return token;
}

static token_t error_token(const char *message)
{
  token_t token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = g_scanner.line;
  return token;
}

static void skip_whitespace()
{
  for(;;) {
    char c = peek();
    switch(c) {
    case ' ':
    case '\r':
    case '\t': advance(); break;
    case '\n':
      g_scanner.line++;
      advance();
      break;
    case '/':
      if(peek_next() == '/') {
        // A comment goes until the end of the line.
        while(peek() != '\n' && !is_at_end()) {
          advance();
        }
      }
      else {
        return;
      }
      break;

    default: return;
    }
  }
}

static token_type_t check_keyword(int start, int length, const char *rest, token_type_t type)
{
  if(g_scanner.current - g_scanner.start == start + length
     && memcmp(g_scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static token_type_t identifier_type()
{
  switch(*g_scanner.start) {
  case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
  case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
  case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if(g_scanner.current - g_scanner.start > 1) {
      switch(g_scanner.start[1]) {
      case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
      case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
      case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
  case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
  case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
  case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
  case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
  case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
  case 't':
    if(g_scanner.current - g_scanner.start > 1) {
      switch(g_scanner.start[1]) {
      case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
      case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
  case 'w': return check_keyword(1, 3, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

static token_t identifier()
{
  while(is_alpha(peek()) || is_digit(peek())) {
    advance();
  }
  return make_token(identifier_type());
}

static token_t number()
{
  while(is_digit(peek())) {
    advance();
  }

  // Look for a fractional part.
  if(peek() == '.' && is_digit(peek_next())) {
    // Consume the "."
    advance();

    while(is_digit(peek())) {
      advance();
    }
  }
  return make_token(TOKEN_NUMBER);
}

static token_t string()
{
  while(peek() != '"' && !is_at_end()) {
    if(peek() == '\n') {
      g_scanner.line++;
    }
    advance();
  }
  if(is_at_end()) {
    return error_token("Unterminated string.");
  }
  // The closing quote.
  advance();
  return make_token(TOKEN_STRING);
}

token_t scan_token()
{
  skip_whitespace();

  g_scanner.start = g_scanner.current;
  if(is_at_end()) {
    return make_token(TOKEN_EOF);
  }

  char c = advance();

  if(is_alpha(c)) {
    return identifier();
  }
  else if(is_digit(c)) {
    return number();
  }

  switch(c) {
  case '(': return make_token(TOKEN_LEFT_PAREN);
  case ')': return make_token(TOKEN_RIGHT_PAREN);
  case '{': return make_token(TOKEN_LEFT_BRACE);
  case '}': return make_token(TOKEN_RIGHT_BRACE);
  case ',': return make_token(TOKEN_COMMA);
  case '.': return make_token(TOKEN_DOT);
  case '-': return make_token(TOKEN_MINUS);
  case '+': return make_token(TOKEN_PLUS);
  case ';': return make_token(TOKEN_SEMICOLON);
  case '*': return make_token(TOKEN_STAR);
  case '/': return make_token(TOKEN_SLASH);

  case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"': return string();
  }

  return error_token("Unexpected character.");
}