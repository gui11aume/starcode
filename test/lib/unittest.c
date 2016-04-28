#include "unittest.h"

// Standard malloc() and realloc() //
extern void *__libc_malloc(size_t);
extern void *__libc_realloc(void *, size_t);
extern void *__libc_calloc(size_t, size_t);
// Private functions // 
void   redirect_stderr (void);
void * run_testset (void *); 
char * sent_to_stderr (void);
void   unit_test_clean (void);
void   unit_test_init (void);
void   unredirect_stderr (void);
void   test_case_clean (void);
void   test_case_init (void);
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
static int    STDERR_OFF;
static int    TEST_CASE_FAILED;


//     Function definitions     //

void
terminate_thread
(
   int sig
)
{

   fprintf(stderr, "caught SIGTERM (interrupting)\n");

   // Label the test case as failed. //
   TEST_CASE_FAILED = 1;

   // Clean it all. //
   unredirect_stderr();
   test_case_clean();

   // Return to main thread.
   pthread_exit(NULL);

}

int
run_unittest
(
         int            argc,
         char        ** argv,
   const test_case_t  * test_case_list[]
)
{

   // Initialize test set. //
   unit_test_init();

   pthread_t tid;

   int nbad = 0;

   // Run test cases in sequential order.
   for (int j = 0 ; test_case_list[j] != NULL ; j++) {

      const test_case_t *test_cases = test_case_list[j];
      for (int i = 0 ; test_cases[i].fixture != NULL ; i++) {

         // Some verbose information. //
         fprintf(stderr, "%s\n", test_cases[i].test_name);

         // Run test case in thread. //
         pthread_create(&tid, NULL, run_testset, (void *) &test_cases[i]);
         pthread_join(tid, NULL);

         nbad += TEST_CASE_FAILED;

      }

   }

   // Clean and return. //
   unit_test_clean();
   return nbad;

}


void *
run_testset
(
   void * data
)
{

   // Unpack argument. //
   const test_case_t test_case = *(test_case_t *) data;

   // -- Initialize -- //

   // Register signal handlers. This will allow execution
   // to return to main thread in case of crash.
   signal(SIGSEGV, terminate_thread);
   
   // Get test name and fixture (test function). //
   fixture_t fixture = test_case.fixture;

   // Initialize variable, empty buffers... //
   test_case_init();
   unredirect_stderr();

   //  -- Run the test -- //
   (*fixture)();
   
   // -- Clean -- //
   unredirect_stderr();
   test_case_clean();

   return NULL;
  

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

   fprintf(DEBUG_DUMP_FILE, "b run_unittest\n");
   fprintf(DEBUG_DUMP_FILE, "run\n");
   fclose(DEBUG_DUMP_FILE);

}


void
test_case_init
(void) 
{

   reset_alloc();

   N_ERROR_MESSAGES = 0;

   TEST_CASE_FAILED = 0;
   STDERR_OFF = 0;

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
(void)
{

   free(STDERR_BUFFER);
   STDERR_BUFFER = NULL;

}
   

void
redirect_stderr
(void)
{
   if (STDERR_OFF) return;
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
   STDERR_OFF = 1;
}


void
unredirect_stderr
(void)
{
   if (!STDERR_OFF) return;
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
   STDERR_OFF = 0;
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
   TEST_CASE_FAILED = 1;
   if (++N_ERROR_MESSAGES > MAX_N_ERROR_MESSAGES + 1) return;
   int switch_stderr = STDERR_OFF;
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
   TEST_CASE_FAILED = 1;
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
