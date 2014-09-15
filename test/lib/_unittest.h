#ifndef _UNITTEST_PRIVATE_HEADER
#define _UNITTEST_PRIVATE_HEADER
// Standard malloc() and realloc() //
extern void *__libc_malloc(size_t);
extern void *__libc_realloc(void *, size_t);
extern void *__libc_calloc(size_t, size_t);
// Private functions // 
int    debug_run_test_case (const test_case_t);
void   redirect_stderr (void);
int    safe_run_test_case (test_case_t); 
char * sent_to_stderr (void);
void   unit_test_clean (void);
void   unit_test_init (void);
void   unredirect_stderr (void);
void   test_case_clean (int);
void   test_case_init (int);
void * fail_prone_calloc (size_t, size_t);
void * fail_prone_malloc (size_t);
void * fail_prone_realloc (void *, size_t);
#endif
