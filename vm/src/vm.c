#include <compiler.h>
#include <debug.h>
#include <stdio.h>
#include <vm.h>

vm_t g_vm;

static void reset_stack() { g_vm.stack_top = g_vm.stack; }

void init_vm() { reset_stack(); }

void free_vm() {}

static interpret_result_t run()
{
#define READ_BYTE() (*g_vm.ip++)
#define READ_CONSTANT() (g_vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)                                                                              \
  do {                                                                                             \
    value_t b = pop();                                                                             \
    value_t a = pop();                                                                             \
    push(a op b);                                                                                  \
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

    case OP_ADD: {
      BINARY_OP(+);
      break;
    }

    case OP_SUBTRACT: {
      BINARY_OP(-);
      break;
    }

    case OP_MULTIPLY: {
      BINARY_OP(*);
      break;
    }

    case OP_DIVIDE: {
      BINARY_OP(/);
      break;
    }

    case OP_NEGATE: {
      value_t a = pop();
      push(-a);
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
  compile(source);
  return INTERPRET_OK;
}

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