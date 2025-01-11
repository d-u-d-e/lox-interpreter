#pragma once
#include <value.h>

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN
} op_code_t;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  int *lines;
  value_array_t constants;
} chunk_t;

void init_chunk(chunk_t *c);
void free_chunk(chunk_t *c);
void write_chunk(chunk_t *c, uint8_t byte, int line);
int add_constant(chunk_t *c, value_t value);