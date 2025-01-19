#pragma once

#include <chunk.h>
#include <object.h>
#include <table.h>
#include <value.h>

#define FAMES_MAX 64
#define STACK_MAX (FAMES_MAX * UINT8_COUNT)

// This represents a single ongoing function call. It is created each time a function is called.
// The slots field points into the VM's value stack at the first slot usable by this function.
typedef struct {
  obj_closure_t *closure;
  uint8_t *ip; // Used to return from function call
  value_t *slots;
} callframe_t;

typedef struct {
  callframe_t frames[FAMES_MAX];
  int frame_count; // number of ongoing function calls

  value_t stack[STACK_MAX];
  value_t *stack_top;
  table_t globals;
  table_t strings; // string interning
  obj_t *objects;
  obj_upvalue_t *open_upvalues;
} vm_t;

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } interpret_result_t;

extern vm_t g_vm;

void init_vm();
void free_vm();
interpret_result_t interpret(const char *source);
void push(value_t value);
value_t pop();