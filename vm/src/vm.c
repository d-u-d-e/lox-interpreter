#include <compiler.h>
#include <debug.h>
#include <memory.h>
#include <object.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vm.h>

vm_t g_vm;

static value_t clock_native(int arg_count, value_t *args)
{
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack()
{
  g_vm.stack_top = g_vm.stack;
  g_vm.frame_count = 0;
  g_vm.open_upvalues = NULL;
}

static void runtime_error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // Print the stack trace.

  for(int i = g_vm.frame_count - 1; i >= 0; i--) {
    callframe_t *frame = &g_vm.frames[i];
    obj_function_t *function = frame->closure->function;
    size_t inst = frame->ip - function->chunk.code - 1; // -1 because ip points to next instruction

    fprintf(stderr, "[line %d] in ", function->chunk.lines[inst]);
    if(function->name == NULL) {
      fprintf(stderr, "script\n");
    }
    else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_stack();
}

static void define_native(const char *name, native_fn_t function)
{
  // Storing the function name and the function itself on the stack prevents the GC from collecting
  // them. Recall that GC can run after any allocation is performed.
  push(OBJ_VAL(copy_string(name, (int)strlen(name))));
  push(OBJ_VAL(new_native(function)));
  table_set(&g_vm.globals, AS_STRING(g_vm.stack[0]), g_vm.stack[1]);
  pop();
  pop();
}

void init_vm()
{
  reset_stack();
  g_vm.objects = NULL;
  g_vm.gray_count = 0;
  g_vm.gray_capacity = 0;
  g_vm.gray_stack = NULL;

  g_vm.bytes_allocated = 0;
  g_vm.next_gc = 1024 * 1024;

  init_table(&g_vm.globals);
  init_table(&g_vm.strings);
  define_native("clock", clock_native);
}

void free_vm()
{
  free_objects();
  free_table(&g_vm.globals);
  free_table(&g_vm.strings);
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

static value_t peek(int distance) { return g_vm.stack_top[-1 - distance]; }

static bool call(obj_closure_t *closure, int arg_count)
{
  // Let's make sure the user passed the right number of arguments.
  if(arg_count != closure->function->arity) {
    runtime_error("Expected %d arguments but got %d.", closure->function->arity, arg_count);
    return false;
  }

  // We need to prepare a new call frame for run(). Enough room?
  if(g_vm.frame_count == FAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  callframe_t *frame = &g_vm.frames[g_vm.frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  // The -1 accounts for the fact that locals (the actual parameters) start at 1.
  // The first slot is reserved for methods...
  frame->slots = g_vm.stack_top - arg_count - 1;
  return true;
}

static bool call_value(value_t value, int arg_count)
{
  if(IS_OBJ(value)) {
    switch(OBJ_TYPE(value)) {
    case OBJ_CLOSURE: {
      // Bare OBJ_FUNCTION objects are never called by the runtime, as they are wrapped in a closure
      // obj.
      return call(AS_CLOSURE(value), arg_count);
    }
    case OBJ_NATIVE: {
      native_fn_t native = AS_NATIVE(value);
      value_t result = native(arg_count, g_vm.stack_top - arg_count);
      g_vm.stack_top -= arg_count + 1;
      push(result);
      return true;
    }

    default: break; // non callable object type
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

static obj_upvalue_t *capture_upvalue(value_t *local)
{
  // Start at the upvalue closest to the top of the stack.
  obj_upvalue_t *prev = NULL;
  obj_upvalue_t *upvalue = g_vm.open_upvalues;
  while(upvalue != NULL && upvalue->location > local) {
    prev = upvalue;
    upvalue = upvalue->next;
  }

  if(upvalue != NULL && upvalue->location == local) {
    // Reuse the same upvalue if it captures the same local variable.
    return upvalue;
  }
  // Two cases:
  // 1. All open upvalues points to locals above the slot we are looking for or the list is empty.
  // 2. We went past the slot we are looking for; since the list is sorted, that means there is no
  // upvalue.

  // Create a new upvalue and insert it into the list in the right position.
  obj_upvalue_t *new = new_upvalue(local);
  new->next = upvalue;
  if(prev == NULL) {
    // List was empty
    g_vm.open_upvalues = new;
  }
  else {
    prev->next = new;
  }
  return new;
}

static void close_upvalue(value_t *last)
{
  while(g_vm.open_upvalues != NULL && g_vm.open_upvalues->location >= last) {
    obj_upvalue_t *upvalue = g_vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    g_vm.open_upvalues = upvalue->next;
  }
}

static bool isFalsey(value_t value)
{
  return IS_NIL(value) || (IS_BOOL(value) && AS_BOOL(value) == false);
}

static void concatenate()
{
  obj_string_t *b = AS_STRING(peek(0));
  obj_string_t *a = AS_STRING(peek(1));
  int new_length = a->length + b->length;
  // NOTE: ALLOCATE can trigger a GC, so operands must be kept on the stack!
  // That's why we use peek() above.
  char *chars = ALLOCATE(char, new_length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(&chars[a->length], b->chars, b->length);
  chars[new_length] = '\0';
  uint32_t hash = hash_string(chars, new_length);
  obj_string_t *interned = table_find_string(&g_vm.strings, chars, new_length, hash);
  // Safe to pop operands
  pop();
  pop();

  if(interned != NULL) {
    // We must keep using the interned string
    push(OBJ_VAL(interned));
  }
  else {
    // It's a new string, so intern it
    obj_string_t *res = allocate_string(chars, new_length, hash);
    push(OBJ_VAL(res));
    table_set(&g_vm.strings, res, NIL_VAL);
  }
  FREE(char, chars);
}

static interpret_result_t run()
{
  callframe_t *frame = &g_vm.frames[g_vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
    disassemble_instruction(&frame->closure->function->chunk,
                            (int)(frame->ip - frame->closure->function->chunk.code));
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

    case OP_POP: {
      pop();
      break;
    }

    case OP_GET_LOCAL: {
      /*
      Next byte holds the argument, i.e. the slot in the stack, starting from the bottom of the
      frame. Notice that while it may seem redundant to push a value already somewhere down in the
      stack it is not, because other instructions look at the stack top. */
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }

    case OP_SET_LOCAL: {
      /*
      Next byte holds the argument, i.e. the slot in the stack, starting from the bottom of
      the frame, where the local lives. Again, no pop since this comes from an expression statement.
      In other words, every expression produces a value. */
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }

    case OP_GET_GLOBAL: {
      obj_string_t *name = READ_STRING();
      value_t value;
      if(!table_get(&g_vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      obj_string_t *name = READ_STRING();
      /* Can redefine globals
      Recall that this instruction comes from a declaration(), not an expression statement
      so we do the pop here.
      Note pop after table set: GC may trigger after a pop(), and during table_set()
      thus mistakenly collecting the string. */
      table_set(&g_vm.globals, name, peek(0));
      pop();
      break;
    }

    case OP_SET_GLOBAL: {
      obj_string_t *name = READ_STRING();
      if(table_set(&g_vm.globals, name, peek(0))) {
        // table_set adds it even if undefined
        table_delete(&g_vm.globals, name);
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      // Recall that this instruction comes from an expression statement, so no pop here
      break;
    }

    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      // The location of the upvalue is in the heap!
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      // The location of the upvalue is in the heap!
      *frame->closure->upvalues[slot]->location = peek(0);
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

    case OP_PRINT: {
      print_value(pop());
      printf("\n");
      break;
    }

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      // Value is not popped, to see why look how logical operators are implemented.
      if(isFalsey(peek(0))) {
        frame->ip += offset;
      }
      break;
    }

    case OP_LOOP: {
      // Basically like OP_JUMP, but the offset is negative.
      // We could have used OP_JUMP, but the trouble is packing the Signed 16 bit integer offset.
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_CALL: {
      uint8_t arg_count = READ_BYTE();
      if(!call_value(peek(arg_count), arg_count)) {
        // No function object on the stack! Exit
        return INTERPRET_RUNTIME_ERROR;
      }
      // There is a function object on the stack!
      // We need to update the current frame since run() uses it.
      frame = &g_vm.frames[g_vm.frame_count - 1];
      break;
    }

    case OP_CLOSURE: {
      obj_function_t *function = AS_FUNCTION(READ_CONSTANT());
      // Note that we wrap the compiled function into a closure object.
      obj_closure_t *closure = new_closure(function);
      push(OBJ_VAL(closure));

      // Fill the upvalue array
      for(int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        // 'frame' refers to the current enclosing function
        if(is_local) {
          closure->upvalues[i] = capture_upvalue(frame->slots + index);
        }
        else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }

      break;
    }

    case OP_CLOSE_UPVALUE: {
      // The variable that needs to be moved to the heap (closed) is on top of the stack.
      close_upvalue(g_vm.stack_top - 1);
      pop(); // Pop the variable that was moved to the heap
      break;
    }

    case OP_RETURN: {
      value_t result = pop(); // The value to be returned to the caller.
      // Before returning from a function, we need to close the open upvalues! The compiler does not
      // call end_scope() after parsing a function declaration.
      // This is the reason close_upvalue does loop until the last stack location for the current
      // frame.
      close_upvalue(frame->slots);
      g_vm.frame_count--;
      if(g_vm.frame_count == 0) {
        // Top level, exit
        pop(); // Pop the script "function"
        return INTERPRET_OK;
      }
      g_vm.stack_top = frame->slots;              // Pop all locals and parameters
      frame = &g_vm.frames[g_vm.frame_count - 1]; // Reset frame pointer
      push(result);                               // Return value on the stack.
      break;
    }

    case OP_CLASS: {
      push(OBJ_VAL(new_class(READ_STRING())));
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

interpret_result_t interpret(const char *source)
{
  obj_function_t *function = compile(source);
  if(function == NULL) {
    // Compile error
    return INTERPRET_COMPILE_ERROR;
  }
  // Function is sort of a "function" representing the top level code

  push(OBJ_VAL(function));
  obj_closure_t *closure = new_closure(function);
  pop(); // Weird? What is this?
  // Well the GC might decide to collect the function object since it is not referenced at this
  // time! This can happen when we do new_closure()
  push(OBJ_VAL(closure));
  call(closure, 0); // It's not a true call! But it initializes the frame.
  return run();
}