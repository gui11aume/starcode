// Build a shared library to intercept calls to `malloc`.
// This allows to set the error rate of `malloc` to simulate
// memory errors to test error recovery.

#include <stddef.h>
extern void *__libc_malloc(size_t size);

double drand48(void);
double perr = 0.0;

void *(*call)(size_t) = __libc_malloc;

void *
malloc(size_t size)
{
   return (*call)(size);
}

void *
fail_prone_malloc(size_t size)
{
   return drand48() < perr ? NULL : __libc_malloc(size);
}

void
set_malloc_failure_rate_to(double p)
{
   perr = p;
   call = fail_prone_malloc;
}

void
reset_malloc(void)
{
   perr = 0;
   call = __libc_malloc;
}
