#include <chunk.h>
#include <common.h>
#include <debug.h>
#include <vm.h>

int main(int argc, char *argv[])
{
  init_vm();

  chunk_t chunk;
  init_chunk(&chunk);

  // - ((1.2 + 3.4) / 5.6)
  int constant = add_constant(&chunk, 1.2);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant, 123);

  constant = add_constant(&chunk, 3.4);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant, 123);

  write_chunk(&chunk, OP_ADD, 123);

  constant = add_constant(&chunk, 5.6);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant, 123);

  write_chunk(&chunk, OP_DIVIDE, 123);
  write_chunk(&chunk, OP_NEGATE, 123);

  write_chunk(&chunk, OP_RETURN, 123);

  interpret(&chunk);
  free_vm();
  free_chunk(&chunk);
  return 0;
}