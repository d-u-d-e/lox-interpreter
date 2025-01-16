#include <common.h>
#include <compiler.h>
#include <object.h>
#include <scanner.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void (*parse_fn)(bool can_assign);

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence_t precedence;
} parse_rule_t;

typedef struct {
  token_t name; // used to compare the current identifier to the name of the variable
  int depth;    // records the scope where the variable was declared
} local_t;

typedef struct {
  local_t locals[UINT8_COUNT];
  int local_count; // tracks how many locals are in scope, i.e. how many array elements are in use
  int scope_depth; // number of blocks sorrounding the current bit of code, zero indicates global
                   // scope
} compiler_t;

parser_t g_parser;
compiler_t *g_compiler = NULL;
chunk_t *g_compiling_chunk;

static parse_rule_t *get_rule(token_type_t type);
static void statement();
static void declaration();
static void expression();

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

static bool check(token_type_t type) { return g_parser.current.type == type; }

static bool match(token_type_t type)
{
  if(check(type)) {
    advance();
    return true;
  }
  return false;
}

static void synchronize()
{
  g_parser.panic_mode = false;

  while(g_parser.current.type != TOKEN_EOF) {
    if(g_parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }

    switch(g_parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN: return;
    default: break;
    }
    advance();
  }
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

static void init_compiler(compiler_t *compiler)
{
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  g_compiler = compiler;
}

static void end_compiler()
{
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if(!g_parser.had_error) {
    disassemble_chunk(curren_chunk(), "code");
  }
#endif
}

static void begin_scope() { g_compiler->scope_depth++; }
static void end_scope()
{
  g_compiler->scope_depth--;
  // Remove all locals that are no longer in scope
  while(g_compiler->local_count > 0
        && g_compiler->locals[g_compiler->local_count - 1].depth > g_compiler->scope_depth) {
    emit_byte(OP_POP);
    g_compiler->local_count--;
  }
}

static void parse_precedence(precedence_t precedence)
{
  advance();
  // When we call a rule, the current token has already been advanced
  parse_fn prefix_rule = get_rule(g_parser.previous.type)->prefix;
  if(prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  while(precedence <= get_rule(g_parser.current.type)->precedence) {
    advance();
    parse_fn infix_rule = get_rule(g_parser.previous.type)->infix;
    if(infix_rule == NULL) {
      break;
    }
    infix_rule(can_assign);
  }

  // Equality token was not consumed by any rule
  if(can_assign && match(TOKEN_EQUAL)) {
    // Variable assignment cannot be performed
    error("Invalid assignment target.");
  }
}

static void binary(bool can_assign)
{
  uint8_t operator_type = g_parser.previous.type;
  parse_rule_t *rule = get_rule(operator_type);
  parse_precedence((precedence_t)(rule->precedence + 1));

  switch(operator_type) {
  case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
  case TOKEN_BANG_EQUAL: emit_bytes(OP_EQUAL, OP_NOT); break;
  case TOKEN_GREATER: emit_byte(OP_GREATER); break;
  case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
  case TOKEN_LESS: emit_byte(OP_LESS); break;
  case TOKEN_LESS_EQUAL: emit_bytes(OP_GREATER, OP_NOT); break;
  case TOKEN_PLUS: emit_byte(OP_ADD); break;
  case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
  case TOKEN_STAR: emit_byte(OP_MULTIPLY); break;
  case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
  default: return; // unreachable
  }
}

static void literal(bool can_assign)
{
  switch(g_parser.previous.type) {
  case TOKEN_TRUE: emit_byte(OP_TRUE); break;
  case TOKEN_FALSE: emit_byte(OP_FALSE); break;
  case TOKEN_NIL: emit_byte(OP_NIL); break;
  default: return; // unreachable
  }
}

static void number(bool can_assign)
{
  double value = strtod(g_parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void string(bool can_assign)
{
  emit_constant(OBJ_VAL(copy_string(g_parser.previous.start + 1, g_parser.previous.length - 2)));
}

static uint8_t identifier_constant(token_t *token)
{
  return make_constant(OBJ_VAL(copy_string(token->start, token->length)));
}

static bool identifier_equal(token_t *a, token_t *b)
{
  if(a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(compiler_t *compiler, token_t *name)
{
  // Walk backwards to ensure inner locals correctly shadow outer locals
  for(int i = compiler->local_count - 1; i >= 0; i--) {
    local_t *local = &compiler->locals[i];
    if(identifier_equal(name, &local->name)) {
      // Is the local variable already initialized?
      if(local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      // Return the index where the local variable is found
      return i;
    }
  }
  return -1;
}

static void add_local(token_t name)
{
  // Check if there's enough space to add a new local
  if(g_compiler->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  local_t *local = &g_compiler->locals[g_compiler->local_count++];
  local->name = name;
  local->depth = -1; // this means that the variable has not been initialized
}

static void declare_variable()
{
  // This is where the compiler records the existence of a variable
  if(g_compiler->scope_depth == 0) {
    // We save only locals
    return;
  }
  token_t *name = &g_parser.previous;

  /*
  We cannot allow the following code

    {
      var a = "first";
      var a = "second";
    }
  */

  for(int i = g_compiler->local_count - 1; i >= 0; i--) {
    local_t *local = &g_compiler->locals[i];
    /*
    If a local variable is owned by a parent scope, exit
    local->depth != -1 is a sentinel value that indicates that the variable has been initialized,
    so not valid here */
    if(local->depth != -1 && local->depth < g_compiler->scope_depth) {
      break;
    }
    if(identifier_equal(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  add_local(*name);
}

static uint8_t parse_variable(const char *error_message)
{
  consume(TOKEN_IDENTIFIER, error_message);
  declare_variable();
  if(g_compiler->scope_depth > 0) {
    // Dummy index for locals
    return 0;
  }
  // Only globals are looked up by name in the constant table
  return identifier_constant(&g_parser.previous);
}

static void mark_initialized()
{
  // Change the sentinel value of -1 to the actual depth
  g_compiler->locals[g_compiler->local_count - 1].depth = g_compiler->scope_depth;
}

static void define_variable(uint8_t global)
{
  // Skip local variables, its value sits on top of the stack and that slot becomes the local
  if(g_compiler->scope_depth > 0) {
    // We only need to mark it initialized
    mark_initialized();
    return;
  }
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void named_variable(token_t token, bool can_assign)
{
  // Check if the variable is a local or a global
  uint8_t get_op, set_op;
  int arg = resolve_local(g_compiler, &token);
  if(arg != -1) {
    // Found local
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  }
  else {
    // Found global
    arg = identifier_constant(&token);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  // Check if the variable is assigned
  if(can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(set_op, (uint8_t)arg);
  }
  else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}

static void variable(bool can_assign) { named_variable(g_parser.previous, can_assign); }

static void var_declaration()
{
  uint8_t global = parse_variable("Expect variable name.");
  if(match(TOKEN_EQUAL)) {
    expression();
  }
  else {
    emit_byte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  define_variable(global);
}

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void block()
{
  while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void expression_statement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

static void print_statement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

static void statement()
{
  if(match(TOKEN_PRINT)) {
    print_statement();
  }
  else if(match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  }
  else {
    expression_statement();
  }
}

static void declaration()
{
  if(match(TOKEN_VAR)) {
    var_declaration();
  }
  else {
    statement();
  }

  if(g_parser.panic_mode) {
    synchronize();
  }
}

static void grouping(bool can_assign)
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool can_assign)
{
  token_type_t operator_type = g_parser.previous.type;

  // Compile the operand
  parse_precedence(PREC_UNARY);

  switch(operator_type) {
  case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
  case TOKEN_BANG: emit_byte(OP_NOT); break;
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
  [TOKEN_BANG]          = {unary,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,      binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,      binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,      binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,      binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,      binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,      binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,  NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,    NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,    NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,   NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,      NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,   NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,      NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,      NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,   NULL,   PREC_NONE},
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
  compiler_t compiler;
  init_compiler(&compiler);

  g_compiling_chunk = chunk;
  g_parser.had_error = false;
  g_parser.panic_mode = false;

  advance();

  while(!match(TOKEN_EOF)) {
    declaration();
  }

  end_compiler();
  return !g_parser.had_error;
}