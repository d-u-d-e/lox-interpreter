#include <common.h>
#include <compiler.h>
#include <memory.h>
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
  token_t name;     // used to compare the current identifier to the name of the variable
  int depth;        // records the scope where the variable was declared
  bool is_captured; // true if the variable is captured by a later nested function (closure)
} local_t;

// Upvalues refer to local variables in an enclosing function
typedef struct {
  uint8_t index;
  bool is_local;
} upvalue_t;

// Is the compiler compiling a function or the top level script?
typedef enum {
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT,
} function_type_t;

typedef struct compiler_t {
  // The enclosing compiler, if any, for the sorrounding function.
  struct compiler_t *enclosing;

  // The current chunk is always the chunk owned by the function we're in the middle of
  // compiling.
  obj_function_t *function;
  function_type_t type;

  local_t locals[UINT8_COUNT];
  upvalue_t upvalues[UINT8_COUNT]; // There's a restriction on how many unique variables a function
                                   // can close over.
  int local_count; // tracks how many locals are in scope, i.e. how many array elements are in use
  int scope_depth; // number of blocks sorrounding the current bit of code, zero indicates global
                   // scope
} compiler_t;

typedef struct class_compiler {
  struct class_compiler *enclosing;
} class_compiler_t;

parser_t g_parser;
compiler_t *g_current_compiler = NULL;
class_compiler_t *g_current_class = NULL; // Innermost class being compiled

static parse_rule_t *get_rule(token_type_t type);
static void statement();
static void declaration();
static void expression();

static chunk_t *current_chunk() { return &g_current_compiler->function->chunk; }
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

static void emit_byte(uint8_t byte) { write_chunk(current_chunk(), byte, g_parser.previous.line); }
static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
  emit_byte(byte1);
  emit_byte(byte2);
}

static int emit_jump(uint8_t instruction)
{
  emit_byte(instruction);
  // Here is the placeholder
  emit_byte(0xFF);
  emit_byte(0xFF);
  // Return the offset of the placeholder in order to patch it later
  return current_chunk()->count - 2;
}

static void emit_loop(int loop_start)
{
  emit_byte(OP_LOOP);
  int offset = current_chunk()->count - loop_start + 2;
  if(offset > UINT16_MAX) {
    error("Loop body too large.");
  }
  emit_byte((offset >> 8) & 0xFFU);
  emit_byte(offset & 0xFFU);
}

static void emit_return()
{
  if(g_current_compiler->type == TYPE_INITIALIZER) {
    emit_bytes(OP_GET_LOCAL, 0); // Return `this`
  }
  else {
    emit_byte(OP_NIL);
  }
  emit_byte(OP_RETURN);
}

static uint8_t make_constant(value_t value)
{
  // The constant may be heap-allocated
  int constant = add_constant(current_chunk(), value);
  if(constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}
static void emit_constant(value_t value) { emit_bytes(OP_CONSTANT, make_constant(value)); }

static void patch_jump(int offset)
{
  // The jump value is added to the instruction pointer, so we subtract 2
  int jump = current_chunk()->count - offset - 2;
  if(jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  // Big endian
  current_chunk()->code[offset] = (jump >> 8) & 0xFFU;
  current_chunk()->code[offset + 1] = jump & 0xFFU;
}

static void init_compiler(compiler_t *compiler, function_type_t type)
{
  compiler->enclosing = g_current_compiler;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->function = new_function();
  g_current_compiler = compiler;

  if(type != TYPE_SCRIPT) {
    // Previous token is the function's name
    compiler->function->name = copy_string(g_parser.previous.start, g_parser.previous.length);
  }

  local_t *local = &compiler->locals[compiler->local_count++];
  local->depth = 0;
  local->is_captured = false;
  if(type == TYPE_METHOD || type == TYPE_INITIALIZER) {
    // For methods, stack slot 0 is reserved for `this`
    local->name.start = "this";
    local->name.length = 4;
  }
  else {
    // For functions, stack slot 0 is reserved for the function itself (closure), and no user
    // identifier can refer to it, since its name is the empty string
    local->name.start = "";
    local->name.length = 0;
  }
}

static uint8_t identifier_constant(token_t *token)
{
  return make_constant(OBJ_VAL(copy_string(token->start, token->length)));
}

static obj_function_t *end_compiler()
{
  emit_return();
  obj_function_t *function = g_current_compiler->function;

#ifdef DEBUG_PRINT_CODE
  if(!g_parser.had_error) {
    // The implicit function we create for top level code (the script) has no name
    disassemble_chunk(current_chunk(), function->name != NULL ? function->name->chars : "<script>");
  }
#endif

  g_current_compiler = g_current_compiler->enclosing;
  return function;
}

static void begin_scope() { g_current_compiler->scope_depth++; }
static void end_scope()
{
  g_current_compiler->scope_depth--;
  // Remove all locals that are no longer in scope
  while(g_current_compiler->local_count > 0
        && g_current_compiler->locals[g_current_compiler->local_count - 1].depth
             > g_current_compiler->scope_depth) {
    // Pop the local unless it has been captured by a closure
    if(g_current_compiler->locals[g_current_compiler->local_count - 1].is_captured) {
      // At runtime the variable that needs to be moved to the heap (closed) is on top of the stack
      emit_byte(OP_CLOSE_UPVALUE);
    }
    else {
      emit_byte(OP_POP);
    }
    g_current_compiler->local_count--;
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

static uint8_t argument_list()
{
  uint8_t arg_count = 0;
  if(!check(TOKEN_RIGHT_PAREN)) {
    do {
      // Each argument is pushed onto the stack
      expression();
      if(arg_count == 255) {
        error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while(match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

static void call(bool can_assign)
{
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

static void dot(bool can_assign)
{
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifier_constant(&g_parser.previous);

  if(can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_PROPERTY, name);
  }
  else if(match(TOKEN_LEFT_PAREN)) {
    // This is an optimization for method calls. It's pointless to emit OP_GET_PROPERTY followed by
    // OP_CALL.
    uint8_t arg_count = argument_list();
    emit_bytes(OP_INVOKE, name);
    emit_byte(arg_count);
  }
  else {
    emit_bytes(OP_GET_PROPERTY, name);
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

static int add_upvalue(compiler_t *compiler, uint8_t index, bool is_local)
{
  int upvalue_count = compiler->function->upvalue_count;

  // Don't add the same upvalue twice
  for(int i = 0; i < upvalue_count; i++) {
    upvalue_t *upvalue = &compiler->upvalues[i];
    if(upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }

  // Add new upvalue if there's room
  if(upvalue_count == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->function->upvalue_count++;
}

static int resolve_upvalue(compiler_t *compiler, token_t *name)
{
  if(compiler->enclosing == NULL) {
    // We are in top level code compiler, so the variable must be "hopefully" global
    return -1;
  }

  int local = resolve_local(compiler->enclosing, name);
  if(local != -1) {
    // We found a local variable in the enclosing function.
    // This returns the operand of OP_GET_UPVALUE, OP_SET_UPVALUE. The current upvalue index!
    // This way the compiler tracks which variable in the enclosing function needs to be captured.
    compiler->enclosing->locals[local].is_captured = true;
    return add_upvalue(compiler, (uint8_t)local, true);
  }

  // Look for a local variable beyond the enclosing function
  int upvalue = resolve_upvalue(compiler->enclosing, name);
  if(upvalue != -1) {
    return add_upvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

static void add_local(token_t name)
{
  // Check if there's enough space to add a new local
  if(g_current_compiler->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  local_t *local = &g_current_compiler->locals[g_current_compiler->local_count++];
  local->name = name;
  local->depth = -1; // this means that the variable has not been initialized
  local->is_captured = false;
}

static void declare_variable()
{
  // This is where the compiler records the existence of a variable
  if(g_current_compiler->scope_depth == 0) {
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

  for(int i = g_current_compiler->local_count - 1; i >= 0; i--) {
    local_t *local = &g_current_compiler->locals[i];
    /*
    If a local variable is owned by a parent scope, exit
    local->depth != -1 is a sentinel value that indicates that the variable has been initialized,
    so not valid here */
    if(local->depth != -1 && local->depth < g_current_compiler->scope_depth) {
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
  if(g_current_compiler->scope_depth > 0) {
    // Dummy index for locals
    return 0;
  }
  // Only globals are looked up by name in the constant table
  return identifier_constant(&g_parser.previous);
}

static void mark_initialized()
{
  if(g_current_compiler->scope_depth == 0) {
    return;
  }
  // Change the sentinel value of -1 to the actual depth
  g_current_compiler->locals[g_current_compiler->local_count - 1].depth
    = g_current_compiler->scope_depth;
}

static void define_variable(uint8_t global)
{
  // Skip local variables, its value sits on top of the stack and that slot becomes the local
  if(g_current_compiler->scope_depth > 0) {
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
  int arg = resolve_local(g_current_compiler, &token);
  if(arg != -1) {
    // Found local
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  }
  else if((arg = resolve_upvalue(g_current_compiler, &token)) != -1) {
    // We found a local variable in a sorrounding function
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  }
  else {
    // It must be a global
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

static void this_(bool can_assign)
{
  /* `this` is compiled as a local variable
  This is nice, because closures inside methods that reference `this` will correctly capture the
  instance. We check if we are inside a method. */

  if(g_current_class == NULL) {
    error("Can't use 'this' outside of a class.");
    return;
  }

  variable(false);
}

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

static void and_(bool can_assign)
{
  /*
  When this is called, the left expression has already been parsed.
  This means that at runtime, the left expression will be on the stack.
  The LHS is left on the stack if the condition is false, because that is the result of the
  expression. Otherwise it is discarded, and the RHS becomes the value of the expression.
  Now it should be clear why emit_jump does not pop the stack value at runtime.
  */
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(end_jump);
}

static void or_(bool can_assign)
{
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);

  // The LHS is false if we reach here, so pop it.
  patch_jump(else_jump);
  emit_byte(OP_POP);

  // Keep parsing the RHS.
  parse_precedence(PREC_OR);

  // The LHS is true if we reach here, so keep the value on the stack.
  patch_jump(end_jump);
}

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void block()
{
  while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(function_type_t type)
{
  compiler_t compiler;
  init_compiler(&compiler, type);
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

  if(!check(TOKEN_RIGHT_PAREN)) {
    // Parse parameters
    do {
      compiler.function->arity++;
      if(compiler.function->arity > 255) {
        error_at_current("Can't have more than 255 parameters.");
      }
      uint8_t constant = parse_variable("Expect parameter name."); // This will be a local variable
      define_variable(constant);
    } while(match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block(); // Will parse the closing brace

  // No need to do end_scope() because we discard the compiler here (who cares about its locals...)
  obj_function_t *function = end_compiler();

  // At runtime, the function object will be on the stack after parsing its declaration.
  // If it is a global function, it will be followed by a OP_DEFINE_GLOBAL instruction that pops it.
  emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));

  // Emit other closure data; note how OP_CLOSURE has a variably sized encoding.
  for(int i = 0; i < function->upvalue_count; i++) {
    emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
    emit_byte(compiler.upvalues[i].index);
  }
}

static void fun_declaration()
{
  // Functions are first-class, so we parse the name like a variable.
  // Inside a block or other function, a function declaration creates a local variable.
  // At the top level, a function declaration creates a global variable.
  uint8_t global = parse_variable("Expect function name.");
  mark_initialized();      // This way a function can refer to itself in the body.
  function(TYPE_FUNCTION); // Compile the body, leaving the function object on the stack.
  define_variable(global);
}

static void method()
{
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifier_constant(&g_parser.previous);

  // Parse the body
  // This emits the code to create a closure and leave it on top of the stack
  function_type_t type = TYPE_METHOD;
  if(g_parser.previous.length == 4 && memcmp(g_parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }
  function(type);

  emit_bytes(OP_METHOD, constant); // This is the name of the method

  // We need the class to bind the method to!
  // But class_declaration() genereted code to leave the class on the stack, right below the closure!
}

static void expression_statement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

static void if_statement()
{
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  // At runtime this will leave the condition on the stack
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  // We don't know how much to jump, so we leave a placeholder
  // This technique is called backpatching
  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP); // Pop the condition when the condition is truthy
  statement();
  int else_jump = emit_jump(OP_JUMP); // Skip else when the condition is truthy
  patch_jump(then_jump);
  emit_byte(OP_POP); // Pop the condition when the condition is falsey
  // Notice that we emit an implicit else instruction even if there is no else

  // Optional else
  if(match(TOKEN_ELSE)) {
    statement();
  }
  patch_jump(else_jump);
}

static void print_statement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

static void return_statement()
{
  if(g_current_compiler->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if(match(TOKEN_SEMICOLON)) {
    emit_return();
  }
  else {
    // We cannot return a value from an initializer
    if(g_current_compiler->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }

    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emit_byte(OP_RETURN);
  }
}

static void while_statement()
{
  int loop_start = current_chunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP); // Pop the condition when the condition is truthy
  statement();
  emit_loop(loop_start); // Loop back so that we can re-evaluate the condition

  patch_jump(exit_jump);
  emit_byte(OP_POP); // Pop the condition when the condition is falsey
}

static void for_statement()
{
  begin_scope(); // In case the initializer introduces a variable
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  // Parse initializer
  if(match(TOKEN_SEMICOLON)) {
    // No initializer
  }
  else if(match(TOKEN_VAR)) {
    var_declaration();
  }
  else {
    expression_statement();
  }

  // Parse condition
  int loop_start = current_chunk()->count;
  int exit_jump = -1;

  if(!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is falsey
    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); // Condition
  }

  // Parse increment
  if(!match(TOKEN_RIGHT_PAREN)) {
    // Jump after the increment
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = current_chunk()->count;
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  // Parse body
  statement();
  emit_loop(loop_start);

  if(exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP); // Condition
  }

  end_scope();
}

static void class_declaration()
{
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  // Add the class name to the sorrounding function's constant table
  token_t class_name = g_parser.previous;
  uint8_t name_constant = identifier_constant(&g_parser.previous);
  declare_variable();

  emit_bytes(OP_CLASS, name_constant);
  define_variable(name_constant);

  // Track nested classes
  class_compiler_t class_compiler;
  class_compiler.enclosing = g_current_class;
  g_current_class = &class_compiler;

  /* named_variable will generate code to load a variable with the given name on the stack!
  This means that when we execute OP_METHOD, the stack has the method's closure on top, with
  the class right under! The VM cannot assume that the class is global, as Lox support nested
  classes. */
  named_variable(class_name, false);

  // Parse body of class
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  // We no longer need the class object on the stack once methods have been interpreted
  emit_byte(OP_POP);

  // Restore enclosing class
  g_current_class = class_compiler.enclosing;
}

static void statement()
{
  if(match(TOKEN_PRINT)) {
    print_statement();
  }
  else if(match(TOKEN_IF)) {
    if_statement();
  }
  else if(match(TOKEN_RETURN)) {
    return_statement();
  }
  else if(match(TOKEN_WHILE)) {
    while_statement();
  }
  else if(match(TOKEN_FOR)) {
    for_statement();
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
  else if(match(TOKEN_FUN)) {
    fun_declaration();
  }
  else if(match(TOKEN_CLASS)) {
    class_declaration();
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

// Pratt rules: third column is just for infix rules
// clang-format off
parse_rule_t rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,  call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,      NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,      dot,    PREC_CALL},
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
  [TOKEN_AND]           = {NULL,      and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,   NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,      NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,   NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,      or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,      NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,   NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,      NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,      NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,      NULL,   PREC_NONE},
};
// clang-format on

static parse_rule_t *get_rule(token_type_t type) { return &rules[type]; }

obj_function_t *compile(const char *source)
{
  init_scanner(source);
  compiler_t compiler;
  init_compiler(&compiler, TYPE_SCRIPT);

  g_parser.had_error = false;
  g_parser.panic_mode = false;

  advance();

  while(!match(TOKEN_EOF)) {
    declaration();
  }

  obj_function_t *function = end_compiler();
  return g_parser.had_error ? NULL : function;
}

void mark_compiler_roots()
{
  compiler_t *compiler = g_current_compiler;
  while(compiler != NULL) {
    mark_object((obj_t *)compiler->function);
    compiler = compiler->enclosing;
  }
}