#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _UNITTEST_HEADER
#define _UNITTEST_HEADER

//     Constants     //
#define UTEST_BUFFER_SIZE 1024
#define MAX_N_ERROR_MESSAGES 20


//     Type definitions      // 
struct test_case_t;

typedef struct test_case_t test_case_t;
typedef void (*fixture_t)(void);

struct test_case_t {
   char      * test_name;
   fixture_t   fixture;
};


//     Function declarations     //

// Interception of 'malloc()' and 'realloc()' //
void * malloc (size_t size);
void * realloc (void *ptr, size_t size);
void * calloc (size_t nitems, size_t size);
void   set_alloc_failure_rate_to (double);
void   set_alloc_failure_countdown_to (int);
void   reset_alloc (void);

// Functions defined in unittest.c //
char * caught_in_stderr (void);
void   redirect_stderr (void);
void   unredirect_stderr (void);

int
run_unittest
(
         int             argc,
         char         ** argv,
   const test_case_t   * test_cases[]
);

void
assert_fail_critical
(
   const char         * assertion,
   const char         * file,
   const unsigned int   lineno,
   const char         * function
);

void
assert_fail_non_critical
(
   const char         * assertion,
   const char         * file,
   const unsigned int   lineno,
   const char         * function
);

void
debug_fail_dump
(
   const char         * file,
   const unsigned int   lineno,
   const char         * function
);


//    Assert macros    //
#define test_assert(expr)  do { \
        if (expr)  { (void) 0; } \
        else { \
           debug_fail_dump(__FILE__, __LINE__, __func__); \
           assert_fail_non_critical(#expr, __FILE__, __LINE__, __func__); \
	}} while (0)

#define test_assert_critical(expr) do { \
        if (expr)  { (void) 0; } \
        else { \
           debug_fail_dump(__FILE__, __LINE__, __func__); \
           assert_fail_critical(#expr, __FILE__, __LINE__, __func__); \
           return; \
	}} while (0)

#define test_assert_stderr(expr) do { \
        if (strcmp(expr, caught_in_stderr()) == 0)  { (void) 0; } \
        else { \
           debug_fail_dump(__FILE__, __LINE__, __func__); \
           assert_fail_non_critical(#expr, __FILE__, __LINE__, __func__); \
           return; \
	}} while (0)

#endif
