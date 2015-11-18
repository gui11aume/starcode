#include "unittest.h"

// Test cases from tests_trie.c
void test_base_1(void);
void test_base_2(void);
void test_base_3(void);
void test_base_4(void);
void test_base_5(void);
void test_base_6(void);
void test_base_7(void);
void test_base_8(void);
void test_errmsg(void);
void test_search(void);
void test_mem_1(void);
void test_mem_2(void);
void test_mem_3(void);
void test_mem_4(void);
void test_mem_5(void);
void test_mem_6(void);

// Test cases from tests_starcode.c
void test_starcode_1(void);
void test_starcode_2(void);
void test_starcode_3(void);
void test_starcode_4(void);
void test_starcode_5(void);
void test_starcode_6(void);
void test_starcode_7(void);
void test_starcode_8(void);
void test_starcode_9(void);
void test_starcode_10(void);
void test_seqsort(void);

int
main(
   int argc,
   char **argv
)
{

   // Register test cases //
   const static test_case_t test_cases[] = {
      {"trie/base/1", test_base_1},
      {"trie/base/2", test_base_2},
      {"trie/base/3", test_base_3},
      {"trie/base/4", test_base_4},
      {"trie/base/5", test_base_5},
      {"trie/base/6", test_base_6},
      {"trie/base/7", test_base_7},
      {"trie/base/8", test_base_8},
      {"errmsg", test_errmsg},
      {"search", test_search},
      {"mem/1", test_mem_1},
      {"mem/2", test_mem_2},
      {"mem/3", test_mem_3},
      {"mem/4", test_mem_4},
      {"mem/5", test_mem_5},
      {"mem/6", test_mem_6},
      {"starcode/base/1", test_starcode_1},
      {"starcode/base/2", test_starcode_2},
      {"starcode/base/3", test_starcode_3},
      {"starcode/base/4", test_starcode_4},
      {"starcode/base/5", test_starcode_5},
      {"starcode/base/6", test_starcode_6},
      {"starcode/base/7", test_starcode_7},
      {"starcode/base/8", test_starcode_8},
      {"starcode/base/9", test_starcode_9},
      {"starcode/base/10", test_starcode_10},
      {"starcode/seqsort", test_seqsort},
      {NULL, NULL}
   };

   return run_unittest(argc, argv, test_cases);

}
