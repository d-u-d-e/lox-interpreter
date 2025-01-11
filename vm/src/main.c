#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <common.h>
#include <vm.h>

static void repl()
{
  char line[1024];
  for(;;) {
    printf("> ");

    if(!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

static char *read_file(const char *path)
{
  FILE *file = fopen(path, "rb");
  if(!file) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(EX_IOERR);
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char *buffer = malloc(size + 1);
  if(!buffer) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(EX_IOERR);
  }

  size_t result = fread(buffer, 1, size, file);
  if(result != size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(EX_IOERR);
  }
  buffer[size] = '\0';
  fclose(file);
  return buffer;
}

static void run_file(const char *path)
{
  char *source = read_file(path);
  interpret_result_t result = interpret(source);
  free(source);

  if(result == INTERPRET_COMPILE_ERROR) {
    exit(EX_DATAERR);
  }
  else if(result == INTERPRET_RUNTIME_ERROR) {
    exit(EX_SOFTWARE);
  }
}

int main(int argc, char *argv[])
{
  init_vm();

  if(argc == 1) {
    repl();
  }
  else if(argc == 2) {
    run_file(argv[1]);
  }
  else {
    fprintf(stderr, "Usage: %s [script]\n", argv[0]);
    exit(EX_USAGE);
  }

  free_vm();
  return 0;
}