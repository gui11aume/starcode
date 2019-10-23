#include "unittest.h"

int
main
(void)
{

   // Register the test cases from linked files. //
   extern test_case_t test_cases_trie[];
   extern test_case_t test_cases_starcode[];

   const test_case_t *list_of_test_cases[] = {
      test_cases_trie,
      test_cases_starcode,
      NULL, // Sentinel. //
   };

   return run_unittest(list_of_test_cases);

}
