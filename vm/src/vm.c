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

  g_vm.init_string = NULL; // Note this: copy_string can cause GC to run and mark `init_string`,
                           // before it has been initialized
  g_vm.init_string = copy_string("init", 4); // handy reference to string for class initializers

  define_native("clock", clock_native);
}

void free_vm()
{
  g_vm.init_string = NULL;
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
  // For methods, the first slot is reserved for `this`. For functions, it simply holds the closure
  // itself.
  frame->slots = g_vm.stack_top - arg_count - 1;
  return true;
}

static bool call_value(value_t callee, int arg_count)
{
  if(IS_OBJ(callee)) {
    switch(OBJ_TYPE(callee)) {
    case OBJ_CLOSURE: {
      // Bare OBJ_FUNCTION objects are never called by the runtime, as they are wrapped in a closure
      // obj.
      return call(AS_CLOSURE(callee), arg_count);
    }
    case OBJ_NATIVE: {
      native_fn_t native = AS_NATIVE(callee);
      value_t result = native(arg_count, g_vm.stack_top - arg_count);
      g_vm.stack_top -= arg_count + 1;
      push(result);
      return true;
    }

    case OBJ_CLASS: {
      // Create a new instance of the class.
      obj_class_t *klass = AS_CLASS(callee);
      // Replace the class object with the instance object.
      g_vm.stack_top[-arg_count - 1] = OBJ_VAL(new_instance(klass));
      value_t initializer;
      // init_string is basically the string "init".
      if(table_get(&klass->methods, g_vm.init_string, &initializer)) {
        return call(AS_CLOSURE(initializer), arg_count);
      }
      else if(arg_count != 0) {
        // If the class has no initializer, but the user passed arguments, we throw an error.
        runtime_error("Expected 0 arguments but got %d.", arg_count);
        return false;
      }
      return true;
    }

    case OBJ_BOUND_METHOD: {
      obj_bound_method_t *bound = AS_BOUND_METHOD(callee);
      // We need to place the receiver on the stack for the call to work.
      // We replace the current bound method object with the receiver.
      g_vm.stack_top[-arg_count - 1] = bound->receiver;
      return call(bound->method, arg_count);
    }

    default: break; // non callable object type
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

static bool invoke_from_class(obj_class_t *klass, obj_string_t *name, int arg_count)
{
  // This combines the logic of OP_GET_PROPERTY and OP_CALL together.
  value_t method;
  if(!table_get(&klass->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), arg_count);
}

static bool invoke(obj_string_t *name, int arg_count)
{
  value_t receiver = peek(arg_count);

  if(!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have methods.");
    return false;
  }

  obj_instance_t *instance = AS_INSTANCE(receiver);
  // What if the name is a field that is a callable?
  value_t value;
  if(table_get(&instance->fields, name, &value)) {
    g_vm.stack_top[-arg_count - 1] = value; // Replace the receiver with the field value.
    return call_value(value, arg_count);
  }
  return invoke_from_class(instance->klass, name, arg_count);
}

static bool bind_method(obj_class_t *klass, obj_string_t *name)
{
  // Places the bound method on the stack if found.
  value_t method;

  if(!table_get(&klass->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }

  // Wrap the class method in a bound method
  obj_bound_method_t *bound = new_bound_method(peek(0), AS_CLOSURE(method));
  pop(); // Instance
  push(OBJ_VAL(bound));
  return true;
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

static void close_upvalues(value_t *last)
{
  while(g_vm.open_upvalues != NULL && g_vm.open_upvalues->location >= last) {
    obj_upvalue_t *upvalue = g_vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    g_vm.open_upvalues = upvalue->next;
  }
}

static void define_method(obj_string_t *name)
{
  // On the stack we find the closure of the method!
  value_t method = peek(0);
  // Right below the class!
  obj_class_t *klass = AS_CLASS(peek(1));
  table_set(&klass->methods, name, method);
  pop(); // Pop the closure
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

    case OP_GET_PROPERTY: {
      // Only instances have properties.
      if(!IS_INSTANCE(peek(0))) {
        runtime_error("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }

      obj_instance_t *instance = AS_INSTANCE(peek(0));
      obj_string_t *name = READ_STRING();
      value_t value;

      if(table_get(&instance->fields, name, &value)) {
        pop(); // Instance
        push(value);
        break;
      }
      // Fields shadow methods
      if(!bind_method(instance->klass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_SET_PROPERTY: {
      // Only instances have fields. peek(1) because peek(0) is the value
      // we're setting.
      if(!IS_INSTANCE(peek(1))) {
        runtime_error("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }
      obj_instance_t *instance = AS_INSTANCE(peek(1));
      table_set(&instance->fields, READ_STRING(), peek(0));
      value_t value = pop();
      pop();       // Instance
      push(value); // The result of a setter is the assigned value
      break;
    }

    case OP_EQUAL: {
      value_t b = pop();
      value_t a = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }

    case OP_GET_SUPER: {
      obj_string_t *name = READ_STRING();
      obj_class_t *superclass = AS_CLASS(pop());
      // Now we have the instance on the stack.

      if(!bind_method(superclass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      // Now we have the bound method on the stack.
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
      if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
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
      // There was a function object on the stack!
      // We need to update the current frame since run() uses it.
      frame = &g_vm.frames[g_vm.frame_count - 1];
      break;
    }

    case OP_INVOKE: {
      // Similar to OP_CALL
      obj_string_t *method_name = READ_STRING();
      uint8_t arg_count = READ_BYTE();
      if(!invoke(method_name, arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      // We need to update the current frame since run() uses it.
      frame = &g_vm.frames[g_vm.frame_count - 1];
      break;
    }

    case OP_SUPER_INVOKE: {
      obj_string_t *method_name = READ_STRING();
      uint8_t arg_count = READ_BYTE();
      obj_class_t *superclass = AS_CLASS(pop());
      if(!invoke_from_class(superclass, method_name, arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
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
        // `frame` refers to the current enclosing function
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
      close_upvalues(g_vm.stack_top - 1);
      pop(); // Pop the variable that was moved to the heap
      break;
    }

    case OP_RETURN: {
      value_t result = pop(); // The value to be returned to the caller.
      // Before returning from a function, we need to close the open upvalues! The compiler does not
      // call end_scope() after parsing a function declaration.
      // This is the reason close_upvalues does loop until the last stack location for the current
      // frame.
      close_upvalues(frame->slots);
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

    case OP_INHERIT: {
      value_t superclass = peek(1);

      if(!IS_CLASS(superclass)) {
        runtime_error("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }

      obj_class_t *subclass = AS_CLASS(peek(0));
      /* Inherited methods calls are fast as normal method calls!
      Also note that this instruction is emitted before any OP_METHOD instructions, meaning that
      it won't overwrite subclass methods. That at most happens while parsing the methods. */
      table_add_all(&AS_CLASS(superclass)->methods, &subclass->methods);
      pop(); // Subclass
      // Superclass is popped after parsing the methods..
      break;
    }

    case OP_METHOD: {
      define_method(READ_STRING());
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