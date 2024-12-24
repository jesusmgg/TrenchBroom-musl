#include "execinfo.h"

// Stub functions that do nothing

int backtrace(void **buffer, int size)
{
  return 0;
}

char ** backtrace_symbols(void *const *buffer, int size)
{
  return nullptr;
}

void backtrace_symbols_fd(void *const *buffer, int size, int fd) {}
