#include <compiler.h>
#include <debug.h>
#include <memory.h>
#include <object.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vm.h>

vm_t g_vm;

static void reset_stack() { g_vm.stack_top = g_vm.stack; }

static void runtime_error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t inst = g_vm.ip - g_vm.chunk->code - 1; // -1 because ip points to next instruction
  int line = g_vm.chunk->lines[inst];
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

void init_vm()
{
  reset_stack();
  g_vm.objects = NULL;
}

void free_vm() { free_objects(); }

void push(value_t value)
{
  *g_vm.stack_top = value;
  g_vm.stack_top++;
}

value_t pop()
{
  g_vm.stack_top--;
  return *g_vm.stack_top;
}

static value_t peek(int distance) { return g_vm.stack_top[-1 - distance]; }

static bool isFalsey(value_t value)
{
  return IS_NIL(value) || (IS_BOOL(value) && AS_BOOL(value) == false);
}

static void concatenate()
{
  obj_string_t *b = AS_STRING(pop());
  obj_string_t *a = AS_STRING(pop());
  int new_length = a->length + b->length;
  char *chars = ALLOCATE(char, new_length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[new_length] = '\0';
  obj_string_t *result = take_string(chars, new_length);
  push(OBJ_VAL(result));
}

static interpret_result_t run()
{
#define READ_BYTE() (*g_vm.ip++)
#define READ_CONSTANT() (g_vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op)                                                                  \
  do {                                                                                             \
    if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                                               \
      runtime_error("Operands must be numbers.");                                                  \
      return INTERPRET_RUNTIME_ERROR;                                                              \
    }                                                                                              \
    double b = AS_NUMBER(pop());                                                                   \
    double a = AS_NUMBER(pop());                                                                   \
    push(value_type(a op b));                                                                      \
  } while(false)

  for(;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for(value_t *slot = g_vm.stack; slot < g_vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(g_vm.chunk, (int)(g_vm.ip - g_vm.chunk->code));
#endif

    uint8_t instruction = READ_BYTE();
    switch(instruction) {
    case OP_CONSTANT: {
      value_t constant = READ_CONSTANT();
      push(constant);
      break;
    }

    case OP_NIL: {
      push(NIL_VAL);
      break;
    }

    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }

    case OP_FALSE: {
      push(BOOL_VAL(false));
      break;
    }

    case OP_EQUAL: {
      value_t b = pop();
      value_t a = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }

    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }

    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }

    case OP_ADD: {
      if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      }
      else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else {
        runtime_error("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }

    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }

    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }

    case OP_NOT: {
      push(BOOL_VAL(isFalsey(pop())));
      break;
    }

    case OP_NEGATE: {
      if(!IS_NUMBER(peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    }

    case OP_RETURN: {
      print_value(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

interpret_result_t interpret(const char *source)
{
  chunk_t chunk;
  init_chunk(&chunk);

  if(!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  g_vm.chunk = &chunk;
  g_vm.ip = g_vm.chunk->code;
  interpret_result_t result = run();

  free_chunk(&chunk);
  return result;
}