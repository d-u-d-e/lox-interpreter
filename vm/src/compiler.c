#include <common.h>
#include <compiler.h>
#include <scanner.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include <debug.h>
#endif

typedef struct {
  token_t current;
  token_t previous;
  bool had_error;
  bool panic_mode;
} parser_t;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY,
} precedence_t;

typedef void (*parse_fn)(void);

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence_t precedence;
} parse_rule_t;

parser_t g_parser;
chunk_t *g_compiling_chunk;

static chunk_t *curren_chunk() { return g_compiling_chunk; }
static void error_at(token_t *token, const char *message)
{
  // Suppress error messages after a panic
  if(g_parser.panic_mode) {
    return;
  }
  g_parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  if(token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  }
  else if(token->type == TOKEN_ERROR) {
    // Nothing
  }
  else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  g_parser.had_error = true;
}

static void error_at_current(const char *message) { error_at(&g_parser.current, message); }
static void error(const char *message) { error_at(&g_parser.previous, message); }

static void advance()
{
  g_parser.previous = g_parser.current;

  for(;;) {
    g_parser.current = scan_token();
    if(g_parser.current.type != TOKEN_ERROR) {
      break;
    }
    error_at_current(g_parser.current.start);
  }
}

static void consume(token_type_t type, const char *message)
{
  if(g_parser.current.type == type) {
    advance();
    return;
  }
  error_at_current(message);
}

static void emit_byte(uint8_t byte) { write_chunk(curren_chunk(), byte, g_parser.previous.line); }
static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_return() { emit_byte(OP_RETURN); }
static uint8_t make_constant(value_t value)
{
  int constant = add_constant(curren_chunk(), value);
  if(constant > UINT8_MAX) {
    error("Too many constants in chunk.");
    return 0;
  }
  return (uint8_t)constant;
}
static void emit_constant(value_t value) { emit_bytes(OP_CONSTANT, make_constant(value)); }
static void end_compiler()
{
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if(!g_parser.had_error) {
    disassemble_chunk(curren_chunk(), "code");
  }
#endif
}

static parse_rule_t *get_rule(token_type_t type);
static void parse_precedence(precedence_t precedence)
{
  advance();
  parse_fn prefix_rule = get_rule(g_parser.previous.type)->prefix;
  if(prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }
  prefix_rule();

  while(precedence <= get_rule(g_parser.current.type)->precedence) {
    advance();
    parse_fn infix_rule = get_rule(g_parser.previous.type)->infix;
    if(infix_rule == NULL) {
      break;
    }
    infix_rule();
  }
}

static void binary()
{
  uint8_t operator_type = g_parser.previous.type;
  parse_rule_t *rule = get_rule(operator_type);
  parse_precedence((precedence_t)(rule->precedence + 1));

  switch(operator_type) {
  case TOKEN_PLUS: emit_byte(OP_ADD); break;
  case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
  case TOKEN_STAR: emit_byte(OP_MULTIPLY); break;
  case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
  default: return; // unreachable
  }
}

static void number()
{
  double value = strtod(g_parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void grouping()
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary()
{
  token_type_t operator_type = g_parser.previous.type;

  // Compile the operand
  parse_precedence(PREC_UNARY);

  switch(operator_type) {
  case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
  default: return; // unreachable
  }
}

// Pratt rules
// clang-format off
parse_rule_t rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,  NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,      NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,     binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,      binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,      NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,      binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,      binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,      NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,      NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,      NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,      NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,      NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,      NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,      NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,    NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,      NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,      NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,      NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,      NULL,   PREC_NONE},
};
// clang-format on

static parse_rule_t *get_rule(token_type_t type) { return &rules[type]; }

bool compile(const char *source, chunk_t *chunk)
{
  init_scanner(source);

  g_compiling_chunk = chunk;
  g_parser.had_error = false;
  g_parser.panic_mode = false;

  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");

  end_compiler();
  return !g_parser.had_error;
}