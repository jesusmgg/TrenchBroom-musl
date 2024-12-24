#ifndef _EXECINFO_H_
#define _EXECINFO_H_

int backtrace(void **, int);
char ** backtrace_symbols(void *const *, int);
void backtrace_symbols_fd(void *const *, int, int);

#endif /* _EXECINFO_H_ */
