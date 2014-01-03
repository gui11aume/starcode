#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

#define NSTRINGS 4

typedef struct {
   node_t *trie;
} fixture;


const char *string[NSTRINGS] = {
   "AAAAAAAAAAAAAAAAAAAA",
   "AAAAAAAAAAAAAAAAAAAT",
   "TAAAAAAAAAAAAAAAAAAA",
   " AAAAAAAAAAAAAAAAAAA",
};


void reset(narray_t *h) { h->pos = 0; }


// String representation of the trie as a depth first search
// traversal. The stars indicate the a series of with no further
// children. I also use indentation to ease the reading.
char repr[1024];
char trierepr[] =
"AAAAAAAAAAAAAAAAAAAA*"
"T********************"
"TAAAAAAAAAAAAAAAAAAA*********************";

void
strie(
   node_t *node,
   int restart
)
// String representation of the trie.
{
   static int j;
   if (restart) j = 0;
   for (int i = 0 ; i < 5 ; i++) {
      if (node->child[i] != NULL) {
         repr[j++] = untranslate[i];
         strie(node->child[i], 0);
      }
   }
   repr[j++] = '*';
   repr[j] = '\0';
}


void
setup(
   fixture *f,
   gconstpointer test_data
)
// SYNOPSIS:                                                             
//   Construct a very simple trie for testing purposes.                  
{
   f->trie = new_trie(3, 20);
   if (f->trie == NULL) {
      g_error("failed to initialize fixture\n");
   }
   for (int i = 0 ; i < NSTRINGS ; i++) {
      node_t *node = insert_string(f->trie, string[i]);
      // Set node data to non 'NULL'.
      if (node != NULL) {
         node->data = (char *) string[i];
      }
      else {
         g_warning("error during fixture initialization\n");
      }
   }

   // Assert that the trie has the correct structure.
   strie(f->trie, 1);
   g_assert_cmpstr(trierepr, ==, repr);

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


void
test_search(
   fixture *f,
   gconstpointer ignore
)
{

   narray_t *hits = new_narray();
   g_assert(hits != NULL);

   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, hits, 18, 3);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAATA", 3, hits, 3, 15);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAGACTG", 3, hits, 15, 15);
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAAAA", 3, hits, 15, 0);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 19);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAG", 3, hits, 19, 0);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   g_assert_cmpint(hits->pos, ==, 4);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAT", 3, hits, 18, 18);
   g_assert_cmpint(hits->pos, ==, 4);

   free(hits);

}

void
test_mem(
   fixture *f,
   gconstpointer ignore
)
{

   // Test hits dynamic growth. Build a trie with 1000 sequences
   // and search with maxdist of 20 (should return all the sequences).
   char seq[9];
   seq[8] = '\0';
   node_t *trie = new_trie(8, 8);
   g_assert(trie != NULL);
   // Insert 2049 random sequences.
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 8 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(trie, seq);
      g_assert(node != NULL);
      node->data = node;
   }

   narray_t *hits = new_narray();
   g_assert(hits != NULL);
   reset(hits);
   search(trie, "NNNNNNNN", 8, hits, 0, 0);
   g_assert_cmpint(hits->pos, >, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   destroy_trie(trie, NULL);

   // Construct 1000 tries with a `malloc` failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_malloc_failure_rate_to(0.01);

   for (int i = 0 ; i < 1000 ; i++) {
      node_t *trie = new_trie(3, 20);
      if (trie == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         continue;
      }
      for (int j = 0 ; j < NSTRINGS ; j++) {
         node_t *node = insert_string(trie, string[j]);
         // Set node data to non 'NULL'.
         if (node == NULL) {
            g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         }
         else {
            node->data = malloc(sizeof(char));
         }
      }
      destroy_trie(trie, free);
   }

   reset_malloc();

   /*
   // Check that the errors do not go unreported.
   g_assert_cmpint(check_trie_error_and_reset(), >, 0);

   // Test 'search()' 1000 times in a perfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      reset(hits);
      search(f->root, "A", 0, hits);
      g_assert_cmpint(hits->pos, ==, 1);
      g_assert_cmpint(check_trie_error_and_reset(), ==, 0);
   }


   // Test 'search()' in an imperfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(f->root, seq);
      if (node != NULL) node->data = node;
   }

   // Check that the errors do not go unreported.
   g_assert_cmpint(check_trie_error_and_reset(), >, 0);

   reset(hits);
   search(f->root, "NNNNNNNNNNNNNNNNNNNN", 20, hits);

   free(hits);
   reset_malloc();
   */

}


void
test_run
(void)
{
   //FILE *outputf = fopen("out", "w");
   //FILE *inputf = fopen("input_test_file.txt", "r");
   //g_assert(inputf != NULL);

   // Just check that input can be read (test will fail if
   // things go wrong here).
   //starcode(inputf, outputf, 3, 1);
   //fclose(inputf);
   //fclose(outputf);

   FILE *outputf = fopen("out", "w");
   FILE *inputf = fopen("input_test_file_large.txt", "r");
   g_assert(inputf != NULL);
   g_assert(outputf != NULL);
   //g_test_timer_start();
   fprintf(stderr, "\n");
   starcode(inputf, outputf, 3, 1);
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
   g_test_init(&argc, &argv, NULL);
   //g_test_add_func("/trie/sandbox", sandbox);
   g_test_add("/trie/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/test_mem", fixture, NULL, setup, test_mem, teardown);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
