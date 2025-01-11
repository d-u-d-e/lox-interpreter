#include <common.h>
#include <compiler.h>
#include <scanner.h>
#include <stdio.h>

void compile(const char *source)
{
  init_scanner(source);
  int line = -1;
  for(;;) {
    token_t token = scan_token();
    if(line != token.line) {
      printf("%4d ", token.line);
      line = token.line;
    }
    else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if(token.type == TOKEN_EOF) {
      break;
    }
  }
}