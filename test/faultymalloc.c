// Build a shared library to intercept calls to `malloc`.
// This allows to set the error rate of `malloc` to simulate
// memory errors to test error recovery.

#include <stddef.h>
extern void *__libc_malloc(size_t size);
extern void *__libc_realloc(void *, size_t size);

double drand48(void);
double PERR = 0.0;

void *(*MALLOC_CALL)(size_t) = __libc_malloc;
void *(*REALLOC_CALL)(void *, size_t) = __libc_realloc;

void *malloc(size_t size) { return (*MALLOC_CALL)(size); }
void *realloc(void *ptr, size_t size) { return (*REALLOC_CALL)(ptr, size); }

void *
fail_prone_malloc(size_t size)
{
   return drand48() < PERR ? NULL : __libc_malloc(size);
}

void *
fail_prone_realloc(void *ptr, size_t size)
{
   return drand48() < PERR ? NULL : __libc_realloc(ptr, size);
}


void
set_alloc_failure_rate_to(double p)
{
   PERR = p;
   MALLOC_CALL = fail_prone_malloc;
   REALLOC_CALL = fail_prone_realloc;
}

void
reset_alloc(void)
{
   PERR = 0.0;
   MALLOC_CALL = __libc_malloc;
   REALLOC_CALL = __libc_realloc;
}
