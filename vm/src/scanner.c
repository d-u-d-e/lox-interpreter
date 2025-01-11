#include <common.h>
#include <scanner.h>

typedef struct {
  const char *start;
  const char *current;
  int line;
} scanner_t;

scanner_t g_scanner;

void init_scanner(const char *source)
{
  g_scanner.start = source;
  g_scanner.current = source;
  g_scanner.line = 1;
}