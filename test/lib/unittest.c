#include "unittest.h"

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
void * fail_countdown_calloc (size_t, size_t);
void * fail_countdown_malloc (size_t);
void * fail_countdown_realloc (void *, size_t);
void * fail_prone_calloc (size_t, size_t);
void * fail_prone_malloc (size_t);
void * fail_prone_realloc (void *, size_t);


//     Global variables     //
void *(*MALLOC_CALL)(size_t) = __libc_malloc;
void *(*REALLOC_CALL)(void *, size_t) = __libc_realloc;
void *(*CALLOC_CALL)(size_t, size_t) = __libc_calloc;

static double ALLOC_ERR_PROB;
static int    ALLOC_FAIL_COUNTER;
static FILE * DEBUG_DUMP_FILE;
static int    N_ERROR_MESSAGES;
static int    ORIG_STDERR_DESCRIPTOR;
static char * STDERR_BUFFER;
static int  * STDERR_OFF;
static int  * TEST_CASE_FAILED;


//     Function definitions     //

int
run_unittest
(
         int            argc,
         char        ** argv,
   const test_case_t    test_cases[]
)
{

   unit_test_init();
   int debug_mode = argc > 1 && strcmp(argv[1], "--debug") == 0;

   int nbad = 0;
   for (int i = 0 ; ; i++) {
      if (test_cases[i].fixture == NULL) break;
      if (debug_mode)
         nbad += debug_run_test_case(test_cases[i]);
      else
         nbad += safe_run_test_case(test_cases[i]);
   }

   unit_test_clean();
   return nbad;

}


int
debug_run_test_case
(
   const test_case_t test_case
)
{

   // Initialization //
   char * test_name = test_case.test_name;
   fixture_t fixture = test_case.fixture;

   test_case_init(FORK_NO);
   unredirect_stderr();
   fprintf(stderr, "%s\n", test_name);

   // Do the test //
   int test_case_failed = 0;
   (*fixture)();

   if (*TEST_CASE_FAILED) {
      unredirect_stderr();
      fprintf(stderr, "FAILED\n");
      test_case_failed = 1;
   }

   test_case_clean(FORK_NO);
   return test_case_failed;

}


int
safe_run_test_case
(
   const test_case_t test_case
)
{

   int test_case_status;
   pid_t pid;

   // Initialization //
   char * test_name = test_case.test_name;
   fixture_t fixture = test_case.fixture;
   
   test_case_init(FORK_YES);
   unredirect_stderr();
   fprintf(stderr, "%s\n", test_name);

   // Fork //
   if ((pid = fork()) == -1) {
      fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

   if (pid == 0) {
      // Child process. Perform the unit test.
      (*fixture)();
      exit(EXIT_SUCCESS);
   }
   else {
      // Parent process. Wait for child to terminate, then report.
      pid = wait(&test_case_status);

      unredirect_stderr();
      int test_case_failed = 0;

      if (pid == -1) {
         fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
         exit(EXIT_FAILURE);
      }
      if (*TEST_CASE_FAILED || WIFEXITED(test_case_status) == 0) {
         fprintf(stderr, "FAILED\n");
         test_case_failed = 1;
      }

      test_case_clean(FORK_YES);
      return test_case_failed;
  
   }

}


void
unit_test_init
(void)
{

   DEBUG_DUMP_FILE = fopen(".inspect.gdb", "w");
   if (DEBUG_DUMP_FILE == NULL) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

}

void
unit_test_clean
(void)
{

   fprintf(DEBUG_DUMP_FILE, "run --debug\n");
   fclose(DEBUG_DUMP_FILE);

}


void
test_case_init
(
   int run_flags
)
{

   reset_alloc();

   N_ERROR_MESSAGES = 0;

   if (run_flags & FORK_YES) {
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0
#endif
      // Shared variables //
      TEST_CASE_FAILED = mmap(NULL, sizeof(*TEST_CASE_FAILED),
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      STDERR_OFF = mmap(NULL, sizeof(*STDERR_OFF),
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   }
   else {
      TEST_CASE_FAILED = malloc(sizeof(int));
      if (TEST_CASE_FAILED == NULL) {
         fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
         exit(EXIT_FAILURE);
      }
      STDERR_OFF = malloc(sizeof(int));
      if (STDERR_OFF == NULL) {
         fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
         exit(EXIT_FAILURE);
      }
   }

   *TEST_CASE_FAILED = 0;
   *STDERR_OFF = 0;

   ORIG_STDERR_DESCRIPTOR = dup(STDERR_FILENO);
   if (ORIG_STDERR_DESCRIPTOR == -1) {
      fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

   STDERR_BUFFER = calloc(UTEST_BUFFER_SIZE, sizeof(char));
   if (STDERR_BUFFER == NULL) {
      fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

}


void
test_case_clean
(
   int run_flags
)
{

   if (run_flags & FORK_YES) {
      // Shared variables //
      munmap(TEST_CASE_FAILED, sizeof *TEST_CASE_FAILED);
      munmap(STDERR_OFF, sizeof *STDERR_OFF);
   }
   else {
      free(TEST_CASE_FAILED);
      free(STDERR_OFF);
   }
   free(STDERR_BUFFER);
   STDERR_BUFFER = NULL;

}
   

void
redirect_stderr
(void)
{
   if (*STDERR_OFF) return;
   // Flush stderr, redirect to /dev/null and set buffer.
   fflush(stderr);
   int temp = open("/dev/null", O_WRONLY);
   if (temp == -1) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   if (dup2(temp, STDERR_FILENO) == -1) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   memset(STDERR_BUFFER, '\0', UTEST_BUFFER_SIZE * sizeof(char));
   if (setvbuf(stderr, STDERR_BUFFER, _IOFBF, UTEST_BUFFER_SIZE) != 0) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   if (close(temp) == -1) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   fprintf(stderr, "$");
   fflush(stderr);
   *STDERR_OFF = 1;
}


void
unredirect_stderr
(void)
{
   if (!*STDERR_OFF) return;
   fflush(stderr);
   if (dup2(ORIG_STDERR_DESCRIPTOR, STDERR_FILENO) == -1) {
      // Could not restore stderr. No need to give an error
      // message because it will not appear.
      exit(EXIT_FAILURE);
   }
   if (setvbuf(stderr, NULL, _IONBF, 0) != 0) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   *STDERR_OFF = 0;
}


void
assert_fail_non_critical
(
   const char         * assertion,
   const char         * file,
   const unsigned int   lineno,
   const char         * function
)
{
   *TEST_CASE_FAILED = 1;
   if (++N_ERROR_MESSAGES > MAX_N_ERROR_MESSAGES + 1) return;
   int switch_stderr = *STDERR_OFF;
   if (switch_stderr) unredirect_stderr();
   if (N_ERROR_MESSAGES == MAX_N_ERROR_MESSAGES + 1) {
      fprintf(stderr, "more than %d failed assertions...\n",
            MAX_N_ERROR_MESSAGES);
   }
   else {
      fprintf(stderr, "assertion failed in %s(), %s:%d: `%s'\n",
            function, file, lineno, assertion);
   }
   fflush(stderr);
   if (switch_stderr) redirect_stderr();
}


void
assert_fail_critical
(
   const char         * assertion,
   const char         * file,
   const unsigned int   lineno,
   const char         * function
)
{
   *TEST_CASE_FAILED = 1;
   unredirect_stderr();
   fprintf(stderr, "assertion failed in %s(), %s:%d: `%s' (CRITICAL)\n",
         function, file, lineno, assertion);
   fflush(stderr);
}


void
debug_fail_dump
(
   const char         * file,
   const unsigned int   lineno,
   const char         * function
)
{
   fprintf(DEBUG_DUMP_FILE, "b %s:%d\n", file, lineno);
   fprintf(DEBUG_DUMP_FILE, "b %s\n", function);
   fflush(DEBUG_DUMP_FILE);
}


char *
caught_in_stderr
(void)
{
   return STDERR_BUFFER;
}


void *
malloc
(
   size_t size
)
{
   return (*MALLOC_CALL)(size);
}

void *
realloc
(
   void *ptr,
   size_t size
)
{
   return (*REALLOC_CALL)(ptr, size);
}

void *
calloc
(
   size_t nitems,
   size_t size
)
{
   return (*CALLOC_CALL)(nitems, size);
}

void *
fail_countdown_malloc(size_t size)
{
   if (ALLOC_FAIL_COUNTER >= 0) ALLOC_FAIL_COUNTER--;
   return ALLOC_FAIL_COUNTER < 0 ? NULL : __libc_malloc(size);
}

void *
fail_countdown_realloc(void *ptr, size_t size)
{
   if (ALLOC_FAIL_COUNTER >= 0) ALLOC_FAIL_COUNTER--;
   return ALLOC_FAIL_COUNTER < 0 ? NULL : __libc_realloc(ptr, size);
}

void *
fail_countdown_calloc(size_t nitems, size_t size)
{
   if (ALLOC_FAIL_COUNTER >= 0) ALLOC_FAIL_COUNTER--;
   return ALLOC_FAIL_COUNTER < 0 ? NULL : __libc_calloc(nitems, size);
}

void *
fail_prone_malloc(size_t size)
{
   return drand48() < ALLOC_ERR_PROB ? NULL : __libc_malloc(size);
}

void *
fail_prone_realloc(void *ptr, size_t size)
{
   return drand48() < ALLOC_ERR_PROB ? NULL : __libc_realloc(ptr, size);
}

void *
fail_prone_calloc(size_t nitems, size_t size)
{
   return drand48() < ALLOC_ERR_PROB ? NULL : __libc_calloc(nitems, size);
}

void
set_alloc_failure_rate_to(double p)
{
   ALLOC_ERR_PROB = p;
   MALLOC_CALL = fail_prone_malloc;
   REALLOC_CALL = fail_prone_realloc;
   CALLOC_CALL = fail_prone_calloc;
}

void
set_alloc_failure_countdown_to(int count)
{
   ALLOC_FAIL_COUNTER = count;
   MALLOC_CALL = fail_countdown_malloc;
   REALLOC_CALL = fail_countdown_realloc;
   CALLOC_CALL = fail_countdown_calloc;
}

void
reset_alloc
(void)
{
   ALLOC_ERR_PROB = 0.0;
   MALLOC_CALL = __libc_malloc;
   REALLOC_CALL = __libc_realloc;
   CALLOC_CALL = __libc_calloc;
}
