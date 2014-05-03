#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

// -- DECLARATION OF PRIVATE FUNCTIONS FROM trie.c -- //
node_t *new_trienode(char);
void destroy_nodes_downstream_of(node_t*, void(*)(void*));
void init_milestones(node_t*);


typedef struct {
   node_t *trie;
} fixture;


// Convenience functions.
void reset(hstack_t *h) { h->pos = 0; h->err = 0; }
int visit(node_t*, char*, int);
char *to_string (node_t *trie, char* s) { visit(trie, s, 0); return s; }

int
visit
(
   node_t *node,
   char *buff,
   int j
)
{
   for (int i = 0 ; i < 6 ; i++) {
      if (node->child[i] != NULL) {
         buff[j++] = untranslate[i];
         j = visit(node->child[i], buff, j);
      }
   }
   buff[j++] = '*';
   buff[j] = '\0';
   return j;
}


// --  ERROR MESSAGE HANDLING FUNCTIONS  -- //

char ERROR_BUFFER[1024];
int BACKUP_FILE_DESCRIPTOR;

void
redirect_stderr_to
(
   char buffer[]
)
{
   // Flush stderr, redirect to /dev/null and set buffer.
   fflush(stderr);
   int temp = open("/dev/null", O_WRONLY);
   dup2(temp, STDERR_FILENO);
   memset(buffer, '\0', 1024 * sizeof(char));
   setvbuf(stderr, buffer, _IOFBF, 1024);
   close(temp);
   // Fill the buffer (needed for reset).
   fprintf(stderr, "fill the buffer");
   fflush(stderr);
}

void
unredirect_sderr
(void)
{
   fflush(stderr);
   dup2(BACKUP_FILE_DESCRIPTOR, STDERR_FILENO);
   setvbuf(stderr, NULL, _IONBF, 0);
}


// --  SET UP AND TEAR DOWN TEST CASES  -- //

void
setup(
   fixture *f,
   gconstpointer test_data
)
// SYNOPSIS:                                                             
//   Construct a very simple trie for testing purposes.                  
{

   const char *string[22] = {
      "AAAAAAAAAAAAAAAAAAAA",
      "AAAAAAAAAANAAAAAAAAA",
      "NAAAAAAAAAAAAAAAAAAN",
      "NNAAAAAAAAAAAAAAAANN",
      "AAAAAAAAAAAAAAAAAAAT",
      "TAAAAAAAAAAAAAAAAAAA",
      "GGATTAGATCACCGCTTTCG",
      "TTGGTATATGTCATAGAAAT",
      "TTCGGAGCGACTAATATAGG",
      "CGAGGCGTATAGTGTTTCCA",
      "ATGCTAGGGTACTCGATAAC",
      "CATACAGTATTCGACTAAGG",
      "TGGAGATGATGAAGAAGACC",
      "GGGAGACTTTTCCAGGGTAT",
      "TCATTGTGATAGCTCGTAAC",
      " GGATATCAAGGGTTACTAG",
      " AAAAAAAAAAAAAAAAAAA",
      "            AAAAAAAA",
      "            NNNNNNNN",
      "                   A",
      "                   N",
      "                    ",
   };


   f->trie = new_trie(3, 20);
   if (f->trie == NULL || f->trie->data == NULL) {
      g_error("failed to initialize fixture\n");
   }
   for (int i = 0 ; i < 22 ; i++) {
      node_t *node = insert_string(f->trie, string[i]);
      // Set node data to non 'NULL'.
      if (node != NULL) {
         // Set the 'data' pointer to self.
         node->data = node;
      }
      else {
         g_warning("error during fixture initialization\n");
      }
   }


   // String representation of the trie as a depth first search
   // traversal. The stars indicate the a series of with no further
   // children. I also use indentation to ease the reading.
   char trie_string_representation[] =
   "NNAAAAAAAAAAAAAAAANN"
    "*******************"
    "AAAAAAAAAAAAAAAAAAN"
   "********************"
   "AAAAAAAAAANAAAAAAAAA"
             "**********"
             "AAAAAAAAAA"
                      "*"
                      "T"
    "*******************"
    "TGCTAGGGTACTCGATAAC"
   "********************"
   "CATACAGTATTCGACTAAGG"
    "*******************"
    "GAGGCGTATAGTGTTTCCA"
   "********************"
   "GGATTAGATCACCGCTTTCG"
     "******************"
     "GAGACTTTTCCAGGGTAT"
   "********************"
   "TAAAAAAAAAAAAAAAAAAA"
    "*******************"
    "CATTGTGATAGCTCGTAAC"
    "*******************"
    "GGAGATGATGAAGAAGACC"
    "*******************"
    "TCGGAGCGACTAATATAGG"
     "******************"
     "GGTATATGTCATAGAAAT"
   "********************"
   " AAAAAAAAAAAAAAAAAAA"
    "*******************"
    "GGATATCAAGGGTTACTAG"
    "*******************"
    "           NNNNNNNN"
               "********"
               "AAAAAAAA"
               "********"
               "       N"
                      "*"
                      "A"
                      "*"
                      " "
   "********************"
   "*";

   // Assert that the trie has the correct structure.
   char buff[1024];
   to_string(f->trie, buff);
   g_assert_cmpstr(trie_string_representation, ==, buff);
   g_assert_cmpint(count_nodes(f->trie), ==, 338);

   return;

}

void
teardown(
   fixture *f,
   gconstpointer test_data
)
{
   destroy_trie(f->trie, NULL);
}



// --  TEST FUNCTIONS -- //

void
test_search(
   fixture *f,
   gconstpointer ignore
)
{

   hstack_t *hits = new_hstack();
   g_assert(hits != NULL);

   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 18);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, &hits, 18, 3);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAATA", 3, &hits, 3, 15);
   g_assert_cmpint(hits->pos, ==, 5);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAGACTG", 3, &hits, 15, 15);
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAAAA", 3, &hits, 15, 0);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 19);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAG", 3, &hits, 19, 0);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 19);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAT", 3, &hits, 19, 0);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, &hits, 0, 0);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, " TGCTAGGGTACTCGATAAC", 1, &hits, 0, 20);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, "NAAAAAAAAAAAAAAAAAAN", 2, &hits, 0, 1);
   g_assert_cmpint(hits->pos, ==, 5);

   reset(hits);
   search(f->trie, "NNAAAAAAAAAAAAAAAANN", 3, &hits, 1, 0);
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   search(f->trie, "AAAAAAAAAANAAAAAAAAA", 0, &hits, 0, 0);
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   search(f->trie, "NNNAGACTTTTCCAGGGTAT", 3, &hits, 0, 0);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, "GGGAGACTTTTCCAGGGNNN", 3, &hits, 0, 17);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, "GGGAGACTTTTCCAGGG   ", 3, &hits, 17, 0);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, "   AGACTTTTCCAGGGTAT", 3, &hits, 0, 3);
   g_assert_cmpint(hits->pos, ==, 1);

   reset(hits);
   search(f->trie, "                   N", 1, &hits, 3, 19);
   g_assert_cmpint(hits->pos, ==, 3);

   reset(hits);
   search(f->trie, "                    ", 1, &hits, 19, 0);
   g_assert_cmpint(hits->pos, ==, 3);

   // Caution here: the hit is present but the initial
   // search conditions are wrong.
   reset(hits);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, &hits, 20, 1);
   g_assert_cmpint(hits->pos, ==, 0);

   // Repeat first test cases.
   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 18);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, &hits, 18, 18);
   g_assert_cmpint(hits->pos, ==, 6);

   destroy_hstack(hits);
   return;

}

void
test_errmsg
(
   fixture *f,
   gconstpointer ignore
)
{

   char string[1024];
   hstack_t *hits = new_hstack();
   int err = 0;

   // Check error messages in 'new_trie()'.
   redirect_stderr_to(ERROR_BUFFER);
   node_t *trie = new_trie(9, 20);
   unredirect_sderr();
   g_assert(trie == NULL);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: 'maxtau' cannot be greater than 8\n");

   // Check error messages in 'insert()'.
   char too_long_string[M+2];
   too_long_string[M+1] = '\0';
   for (int i = 0 ; i < M+1 ; i++) too_long_string[i] = 'A';
   redirect_stderr_to(ERROR_BUFFER);
   node_t *not_inserted = insert_string(f->trie, too_long_string);
   unredirect_sderr();
   sprintf(string, "error: cannot insert string longer than %d\n",
         MAXBRCDLEN);
   g_assert(not_inserted == NULL);
   g_assert_cmpstr(ERROR_BUFFER, ==, string);
 
  
   // Check error messages in 'search()'.
   redirect_stderr_to(ERROR_BUFFER);
   err = search(f->trie, "AAAAAAAAAAAAAAAAAAAAA", 3, &hits, 1, 0);
   g_assert_cmpint(err, ==, 65);
   unredirect_sderr();
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: query longer than allowed max\n");
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   redirect_stderr_to(ERROR_BUFFER);
   err = search(f->trie, " TGCTAGGGTACTCGATAAC", 4, &hits, 0, 0);
   unredirect_sderr();
   g_assert_cmpint(err, ==, 59);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: requested tau greater than 'maxtau'\n");
   g_assert_cmpint(hits->pos, ==, 0);

   // Force 'malloc()' to fail and check error messages of constructors.
   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   node_t *trie_fail = new_trie(3, 20);
   reset_alloc();
   unredirect_sderr();
   g_assert(trie_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create trie node\n"
      "error: could not create trie\n");

   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   node_t *trienode_fail = new_trienode(3);
   reset_alloc();
   unredirect_sderr();
   g_assert(trienode_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create trie node\n");

   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   narray_t *narray_fail = new_narray();
   reset_alloc();
   unredirect_sderr();
   g_assert(narray_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create node array\n");

   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   hstack_t *hits_fail = new_hstack();
   reset_alloc();
   unredirect_sderr();
   g_assert(hits_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: could not create hit stack\n");

   node_t *dummy_trie = new_trie(3, 20);
   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   init_milestones(dummy_trie);
   reset_alloc();
   unredirect_sderr();
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create node array\n"
      "error: could not initialize trie info\n");
   destroy_trie(dummy_trie, NULL);

   destroy_hstack(hits);
   return;

}

void
test_mem_1(
   fixture *f,
   gconstpointer ignore
)
// NOTE: It may be that the 'g_assert()' functions call 'malloc()'
// in which case I am not sure what may sometimes happen.
{
   // Test hits dynamic growth. Build a trie with 1000 sequences
   // and search with maxdist of 20 (should return all the sequences).
   char seq[9];
   seq[8] = '\0';

   node_t *trie = new_trie(8, 8);
   g_assert(trie != NULL);

   hstack_t *hits = new_hstack();
   g_assert(hits != NULL);
 
   int err = 0;

   // Insert 2049 random sequences.
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 8 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(trie, seq);
      g_assert(node != NULL);
      node->data = node;
   }

   // Search with failure in 'malloc()'.
   set_alloc_failure_rate_to(1);
   redirect_stderr_to(ERROR_BUFFER);
   err = search(trie, "NNNNNNNN", 8, &hits, 0, 8);
   unredirect_sderr();
   reset_alloc();

   g_assert_cmpint(err, >, 0);
   g_assert_cmpint(hits->err, >, 0);

   reset(hits);

   // Do it again without failure.
   err = search(trie, "NNNNNNNN", 8, &hits, 0, 8);
   g_assert_cmpint(err, ==, 0);
   g_assert_cmpint(hits->pos, >, 0);

   destroy_trie(trie, NULL);
   destroy_hstack(hits);
   return;
}


void
test_mem_2(
   fixture *f,
   gconstpointer ignore
)
// NOTE: It may be that the 'g_assert()' functions call 'malloc()'
// in which case I am not sure what may sometimes happen.
{
   // The hit stack 'hits' is properly initialized.
   hstack_t *hits = new_hstack();
   g_assert(hits != NULL);

   char seq[21];
   seq[20] = '\0';

   // Construct 1000 tries with a 'malloc()' failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_alloc_failure_rate_to(0.01);

   redirect_stderr_to(ERROR_BUFFER);
   for (int i = 0 ; i < 1000 ; i++) {
      node_t *trie = new_trie(3, 20);
      if (trie == NULL) {
         // Make sure errors are reported.
         g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         continue;
      }
      for (int j = 0 ; j < 50 ; j++) {
         for (int k = 0 ; k < 20 ; k++) {
            seq[k] = untranslate[(int)(5 * drand48())];
         }
         node_t *node = insert_string(trie, seq);
         if (node == NULL) {
            // Make sure errors are reported.
            g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         }
         else {
            // This can fail, but should not cause problem
            // when the trie is destroyed.
            node->data = malloc(sizeof(char));
         }
      }
      destroy_trie(trie, free);
   }
   unredirect_sderr();
   reset_alloc();
   destroy_hstack(hits);
   return;
}


void
test_mem_3(
   fixture *f,
   gconstpointer ignore
)
// NOTE: It may be that the 'g_assert()' functions call 'malloc()'
// in which case I am not sure what may sometimes happen.
{
   // The variable 'trie' and 'hits' are initialized
   // without 'malloc()' failure.
   node_t *trie = new_trie(3, 20);
   g_assert(trie != NULL);

   hstack_t *hits = new_hstack();
   g_assert(hits != NULL);

   int err = 0;

   // Test 'search()' 1000 times in a perfectly built trie
   // with a 'malloc()' failure rate of 1%.
   set_alloc_failure_rate_to(0.01);

   redirect_stderr_to(ERROR_BUFFER);
   for (int i = 0 ; i < 1000 ; i++) {
      // The following search should not make any call to
      // 'malloc()' and so should have an error code of 0
      // on every call.
      reset(hits);
      err = search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 20);
      g_assert_cmpint(err, == , 0);
      g_assert_cmpint(hits->pos, ==, 6);
   }

   char seq[21];
   seq[20] = '\0';

   // Build an imperfect trie.
   for (int i = 0 ; i < 2048 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(trie, seq);
      if (node == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         node->data = node;
      }
   }

   // Test 'search()' in the imperfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      reset(hits);
      search(trie, seq, 3, &hits, 0, 20);
   }
   unredirect_sderr();
   reset_alloc();
   destroy_hstack(hits);
   return;
}


void
test_mem_4(
   fixture *f,
   gconstpointer ignore
)
// NOTE: It may be that the 'g_assert()' functions call 'malloc()'
// in which case I am not sure what may sometimes happen.
{
   node_t *trie;
   node_t *trienode;
   narray_t *narray;
   hstack_t *hits;

   // Test constructors with a 'malloc()' failure rate of 1%.
   set_alloc_failure_rate_to(0.01);
   redirect_stderr_to(ERROR_BUFFER);

   // Test 'new_trie()'.
   for (int i = 0 ; i < 1000 ; i++) {
      trie = new_trie(3, 20);
      if (trie == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         destroy_trie(trie, NULL);
         trie = NULL;
      }
   }

   // Test 'new_trienode()' (private function from trie.c).
   for (int i = 0 ; i < 1000 ; i++) {
      trienode = new_trienode(3);
      if (trienode == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         destroy_nodes_downstream_of(trienode, NULL);
         trie = NULL;
      }
   }


   // Test 'new_narray()'.
   for (int i = 0 ; i < 1000 ; i++) {
      narray = new_narray();
      if (narray == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         free(narray);
         trie = NULL;
      }
   }

   // Test 'new_hstack()'.
   for (int i = 0 ; i < 1000 ; i++) {
      hits = new_hstack();
      if (hits == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         destroy_hstack(hits);
         hits = NULL;
      }
   }

   unredirect_sderr();
   reset_alloc();
   return;

}


void
test_run
(void)
{
   // Make sure normal 'alloc()' is used throughout.
   reset_alloc();
   //FILE *outputf = fopen("out", "w");
   //FILE *inputf = fopen("input_test_file.txt", "r");
   //g_assert(inputf != NULL);

   // Just check that input can be read (test will fail if
   // things go wrong here).
   //starcode(inputf, outputf, 3, 0, 1);
   //fclose(inputf);
   //fclose(outputf);

   FILE *outputf = fopen("out", "w");
   FILE *inputf = fopen("input_test_file_large.txt", "r");
   g_assert(inputf != NULL);
   g_assert(outputf != NULL);
   //g_test_timer_start();
   fprintf(stderr, "\n");
   //starcode(inputf, outputf, 3, 0, 1);
   fclose(inputf);
   fclose(outputf);
   // The command line call to 'perf' shows the elapsed time.
   //fprintf(stdout, "\nelapsed: %.3f sec\n", g_test_timer_elapsed());
}


int
main(
   int argc,
   char **argv
)
{
   // Save the stderr file descriptor upon start.
   BACKUP_FILE_DESCRIPTOR = dup(STDERR_FILENO);

   g_test_init(&argc, &argv, NULL);
   g_test_add("/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/errmsg", fixture, NULL, setup, test_errmsg, teardown);
   g_test_add("/mem/1", fixture, NULL, setup, test_mem_1, teardown);
   g_test_add("/mem/2", fixture, NULL, setup, test_mem_2, teardown);
   g_test_add("/mem/3", fixture, NULL, setup, test_mem_3, teardown);
   g_test_add("/mem/4", fixture, NULL, setup, test_mem_4, teardown);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
