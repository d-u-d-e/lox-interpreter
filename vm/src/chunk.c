#include <chunk.h>
#include <memory.h>

void init_chunk(chunk_t *chunk)
{
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  init_value_array(&chunk->constants);
}

void free_chunk(chunk_t *c)
{
  FREE_ARRAY(uint8_t, c->code, c->capacity);
  FREE_ARRAY(int, c->lines, c->capacity);
  free_value_array(&c->constants);
  init_chunk(c);
}

void write_chunk(chunk_t *chunk, uint8_t byte, int line)
{
  if(chunk->capacity < chunk->count + 1) {
    // grow chunk
    int old_cap = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_cap);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, old_cap, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int add_constant(chunk_t *c, value_t value)
{
  write_value_array(&c->constants, value);
  // return the index of the constant
  return c->constants.count - 1;
}