#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

#define NSTRINGS 16

typedef struct {
   node_t *root;
} fixture;


const char *string[NSTRINGS] = { "A", "AA", "AAA", "ATA", "ATT", "AAT",
   "TGA", "TTA", "TAT", "TGC", "GGG", "AAAA", "ACAA", "ANAA", 
   "GTATGCTGATGGACC", "GTATGCTGTGGACC" };


void reset(narray_t *h) { h->idx = 0; }


void
sandbox (void)
{
   
   node_t *root = new_trienode();
   narray_t *hits = new_narray();

   g_assert(root != NULL);
   g_assert(hits != NULL);

   char *seq[3] = {
      "AAATTGTTTAACTTGGGTCAAA",
      "AGCCATGCTAGTTGTGGTTTGT",
      "GCCATGCTAGTTGTGGTTTGTC",
   };

   for (int i = 0 ; i < 3 ; i++) {
      node_t *node = insert_string(root, seq[i]);
      g_assert(node != NULL);
      node->data = node;
   }

   search(root, "CCATGCTAGTTGTGGTTTGTCC", 2, hits);
   //g_assert_cmpint(hits->idx, ==, 1);

   free(hits);
   destroy_nodes_downstream_of(root, NULL);

}


// String representation of the triei as a depth first search
// traversal. The stars indicate the a series of with no further
// children. I also use indentation to ease the reading.
char repr[1024];
char trierepr[] =
"ANAA***"
 "AAA**"
  "T**"
 "CAA***"
 "TA*"
  "T***"
"GGG**"
 "TATGCTGATGGACC*******"
        "TGGACC**************"
"TAT**"
 "GA*"
  "C**"
"TA****";


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
   f->root = new_trienode();
   if (f->root == NULL) {
      g_error("failed to initialize fixture\n");
   }
   for (int i = 0 ; i < NSTRINGS ; i++) {
      node_t *node = insert_string(f->root, string[i]);
      // Set node data to non 'NULL'.
      if (node != NULL) {
         node->data = (char *) string[i];
      }
      else {
         g_warning("error during fixture initialization\n");
      }
   }

   // Assert that the trie has the correct structure.
   strie(f->root, 1);
   g_assert_cmpstr(trierepr, ==, repr);

   return;

}


void
teardown(
   fixture *f,
   gconstpointer test_data
)
{
   destroy_nodes_downstream_of(f->root, NULL);
}


void
test_search(
   fixture *f,
   gconstpointer ignore
)
{

   narray_t *hits = new_narray();
   g_assert(hits != NULL);

   // Short queries.
   
   // Case 1. //
   search(f->root, "A", 0, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("A", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 2. //
   reset(hits);
   search(f->root, "AAA", 0, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("AAA", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 3. //
   reset(hits);
   search(f->root, "AAA", 1, hits);
   g_assert_cmpint(hits->idx, ==, 7);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 4. //
   reset(hits);
   search(f->root, "AAA", 2, hits);
   g_assert_cmpint(hits->idx, ==, 12);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 5. //
   reset(hits);
   search(f->root, "TTT", 1, hits);
   g_assert_cmpint(hits->idx, ==, 3);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Long search queries.
   // Perfect matches.
   
   // Case 6. //
   reset(hits);
   search(f->root, "GTATGCTGATGGACC", 0, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 7. //
   reset(hits);
   search(f->root, "GTATGCTGTGGACC", 0, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGTGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // External deletions.
   
   // Case 8. //
   reset(hits);
   search(f->root, "TATGCTGATGGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 9. //
   reset(hits);
   search(f->root, "GTATGCTGATGGAC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 10. //
   reset(hits);
   search(f->root, "TATGCTGATGGAC", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 11. //
   reset(hits);
   search(f->root, "GTATGCTGATGGA", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 12. //
   reset(hits);
   search(f->root, "ATGCTGATGGACC", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 13. //
   reset(hits);
   search(f->root, "TATGCTGATGGA", 3, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 14. //
   reset(hits);
   search(f->root, "ATGCTGATGGAC", 3, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 15. //
   reset(hits);
   search(f->root, "GTATGCTGATGG", 3, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 16. //
   reset(hits);
   search(f->root, "TGCTGATGGACC", 3, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // External and internal deletions.

   // Case 17. //
   reset(hits);
   search(f->root, "TATGCGATGGAC", 3, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Combinations of deletions and mismatches.
   
   // Case 18. //
   reset(hits);
   search(f->root, "TATGCTGATGGACG", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 19. //
   reset(hits);
   search(f->root, "TTATGCTGATGGAC", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 20. //
   reset(hits);
   search(f->root, "GAGTGCAGATGGAGT", 5, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 21. //
   reset(hits);
   search(f->root, "GAGTGCAGATGGAGT", 6, hits);
   g_assert_cmpint(hits->idx, ==, 2);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // One mismatch, several hits.
   
   // Case 22. //
   reset(hits);
   search(f->root, "GTATGCTGATGGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 2);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Queries with "N".
   
   // Case 23. //
   reset(hits);
   search(f->root, "GTATGNTGATGGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 24. //
   reset(hits);
   search(f->root, "NTATGCTGATGGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 25. //
   reset(hits);
   search(f->root, "GTATGCTGATGGACN", 1, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 26. //
   reset(hits);
   search(f->root, "NNATGCTGATGGACC", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 27. //
   reset(hits);
   search(f->root, "GTATGCTGATGGANN", 2, hits);
   g_assert_cmpint(hits->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, (char *) hits->nodes[0]->data);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 28. //
   reset(hits);
   search(f->root, "GTATGCTGNTGGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 2);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Queries with no match.

   // Case 29. //
   reset(hits);
   search(f->root, "GAGTGCAGATGGAGT", 4, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 30. //
   reset(hits);
   search(f->root, "NNNNNNNN", 4, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 31. //
   reset(hits);
   search(f->root, "NNNNNNNN", 7, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 32. //
   reset(hits);
   search(f->root, "ANAA", 0, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 33. //
   reset(hits);
   search(f->root, "GATCGATC", 4, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Case 34. //
   reset(hits);
   search(f->root, "GTAGCTGATGACC", 1, hits);
   g_assert_cmpint(hits->idx, ==, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

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
   char seq[21];
   seq[20] = '\0';
   node_t *root = new_trienode();
   g_assert(root != NULL);
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(root, seq);
      g_assert(node != NULL);
      node->data = node;
   }

   narray_t *hits = new_narray();
   g_assert(hits != NULL);
   reset(hits);
   search(root, "NNNNNNNNNNNNNNNNNNNA", 20, hits);
   g_assert_cmpint(hits->idx, >, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   destroy_nodes_downstream_of(root, NULL);

   // Construct 1000 tries with a `malloc` failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_malloc_failure_rate_to(0.01);

   for (int i = 0 ; i < 1000 ; i++) {
      node_t *root = new_trienode();
      if (root == NULL) continue;
      for (int j = 0 ; j < NSTRINGS ; j++) {
         node_t *node = insert_string(root, string[j]);
         // Set node data to non 'NULL'.
         if (node != NULL) node->data = malloc(sizeof(char));
      }
      destroy_nodes_downstream_of(root, free);
   }

   // Check that the errors do not go unreported.
   g_assert_cmpint(check_trie_error_and_reset(), >, 0);

   // Test 'search()' 1000 times in a perfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      reset(hits);
      search(f->root, "A", 0, hits);
      g_assert_cmpint(hits->idx, ==, 1);
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

}



void
test_run
(void)
{
   FILE *outputf = fopen("/dev/null", "w");
   FILE *inputf = fopen("input_test_file.txt", "r");
   g_assert(inputf != NULL);

   // Just check that input can be read (test will fail if
   // things go wrong here).
   starcode(inputf, outputf, 2, 0);
   fclose(inputf);
   fclose(outputf);

   outputf = fopen("out", "w");
   inputf = fopen("input_test_file_large.txt", "r");
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
