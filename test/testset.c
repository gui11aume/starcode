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

void init_DYNP(void);
int dist_less_than(int, node_t *, node_t *);

const char *string[NSTRINGS] = { "A", "AA", "AAA", "ATA", "ATT", "AAT",
   "TGA", "TTA", "TAT", "TGC", "GGG", "AAAA", "ACAA", "ANAA", 
   "GTATGCTGATGGACC", "GTATGCTGTGGACC" };

char *
node_to_string
(
   node_t *node,
   char *buff
)
{
   buff[node->depth] = '\0';
   for (int i = node->depth-1 ; node->parent != NULL ; i--) {
      buff[i] = untranslate[node->pos];
      node = node->parent;
   }
   return buff;
}


void
pairs
(
   node_t *node
)
// SYNOPSIS:                                                              
//   Prototype.                                                           
{
   char str1[M];
   char str2[M];
   if (node->data == NULL) return;

   nstack_t *active_set = node->ans;
   for (int i = 0 ; i < active_set->idx ; i++) {
      node_t *active_node = active_set->node[i];
      if (active_node->data != NULL) {
         //fprintf(stderr, "%s,%s\n",
         //      node_to_string(node, str1),
         //      node_to_string(active_node, str2));
      }
   }
}


void
sandbox (void)
{
   
   node_t *root = new_trienode();

   g_assert(root != NULL);

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

   //search(root, "CCATGCTAGTTGTGGTTTGTCC", 2);

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
   init_DYNP();

   f->root = new_trienode();
   if (f->root == NULL) {
      g_error("failed to initialize fixture\n");
   }
   for (int i = 0 ; i < NSTRINGS ; i++) {
      node_t *node = insert_string(f->root, string[i]);
      // Set node data to non 'NULL'.
      if (node != NULL) {
         node->data = node;
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
test_dist_less_than
(
   fixture *f,
   gconstpointer ignore
)
{
   node_t *node1 = f->root;
   node_t *node2 = f->root;
   int P1[15] = {3,4,1,4,3,2,4,3,1,4,3,3,1,2,2};
   int P2[14] = {3,4,1,4,3,2,4,3,4,3,3,1,2,2};
   for (int i = 0 ; i < 15 ; i++) node1 = node1->child[P1[i]];
   for (int i = 0 ; i < 14 ; i++) node2 = node2->child[P2[i]];

   g_assert(node1 != NULL);
   g_assert(node2 != NULL);

   g_assert(dist_less_than(1, node1, node2));
   g_assert(check_trie_error_and_reset() == 0);

}


void
test_triestack
(
   fixture *f,
   gconstpointer ignore
)
{
   //triestack(f->root, 0, pairs);
   //triestack(f->root, 1, pairs);
   //triestack(f->root, 2, pairs);
   triestack(f->root, 3, pairs);
}

/*
void
test_search
(
   fixture *f,
   gconstpointer ignore
)
{

   FILE *debugf = fopen("debug.txt", "w");
   printrie(debugf, f->root);
   fclose(debugf);


   // Short queries.

   // Case 1. //
   mset = search(f->root, "A", 0);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpint(mset->matches[0].dist, == , 0);
   g_assert_cmpstr("A", ==, mset->matches[0].seq);
   free(mset);


   // Case 2. //
   mset = search(f->root, "AAA", 0);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpint(mset->matches[0].dist, == , 0);
   g_assert_cmpstr("AAA", ==, mset->matches[0].seq);
   free(mset);


   // Case 3. //
   mset = search(f->root, "AAA", 1);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 7);
   int dist_3[7] = {0,1,1,1,1,1,1};
   for (int i = 0 ; i < 7 ; i++) {
      g_assert_cmpint(mset->matches[i].dist, == , dist_3[i]);
   }
   free(mset);


   // Case 4. //
   mset = search(f->root, "AAA", 2);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 12);
   int dist_4[12] = {0,1,1,1,1,1,1,2,2,2,2,2};
   for (int i = 0 ; i < 12 ; i++) {
      g_assert_cmpint(mset->matches[i].dist, == , dist_4[i]);
   }
   free(mset);


   // Case 5. //
   mset = search(f->root, "TTT", 1);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 3);
   int dist_5[3] = {1,1,1};
   for (int i = 0 ; i < 3 ; i++) {
      g_assert_cmpint(mset->matches[i].dist, == , dist_5[i]);
   }
   free(mset);

   // Long search queries.
   // Perfect matches.
   
   // Case 6. //
   mset = search(f->root, "GTATGCTGATGGACC", 0);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, mset->matches[0].seq);
   free(mset);


   // Case 7. //
   mset = search(f->root, "GTATGCTGTGGACC", 0);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGTGGACC", ==, mset->matches[0].seq);
   free(mset);


   // External deletions.
   
   // Case 8. //
   mset = search(f->root, "TATGCTGATGGACC", 1);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, mset->matches[0].seq);
   free(mset);

   // Case 9. //
   mset = search(f->root, "GTATGCTGATGGAC", 1);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, mset->matches[0].seq);
   free(mset);

   // Case 10. //
   mset = search(f->root, "TATGCTGATGGAC", 2);
   g_assert_cmpint(check_and_reset_trie_error(), ==, 0);
   g_assert(mset != NULL);
   g_assert_cmpint(mset->idx, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, mset->matches[0].seq);
   free(mset);

   fprintf(stderr, "control\n");

   // Case 11. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGATGGA", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 12. //
//   clear_hip(f->hip);
   search(f->root, "ATGCTGATGGACC", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 13. //
//   clear_hip(f->hip);
   search(f->root, "TATGCTGATGGA", 3);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 3);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 14. //
//   clear_hip(f->hip);
   search(f->root, "ATGCTGATGGAC", 3);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 3);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 15. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGATGG", 3);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 3);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 16. //
//   clear_hip(f->hip);
   search(f->root, "TGCTGATGGACC", 3);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 3);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // External and internal deletions.

   // Case 17. //
//   clear_hip(f->hip);
   search(f->root, "TATGCGATGGAC", 3);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 3);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Combinations of deletions and mismatches.
   
   // Case 18. //
//   clear_hip(f->hip);
   search(f->root, "TATGCTGATGGACG", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 19. //
//   clear_hip(f->hip);
   search(f->root, "TTATGCTGATGGAC", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 20. //
//   clear_hip(f->hip);
   search(f->root, "GAGTGCAGATGGAGT", 5);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 5);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 21. //
//   clear_hip(f->hip);
   search(f->root, "GAGTGCAGATGGAGT", 6);
//   g_assert_cmpint(f->hip->n_hits, ==, 2);
//   int dist_21[2] = {5,6};
   for (int i = 0 ; i < 2 ; i++) {
//      g_assert_cmpint(f->hip->hits[i].dist, ==, dist_21[i]);
   }
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // One mismatch, several hits.
   
   // Case 22. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGATGGACC", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 2);
//   int dist_22[2] = {0,1};
   for (int i = 0 ; i < 2 ; i++) {
//      g_assert_cmpint(f->hip->hits[i].dist, ==, dist_22[i]);
   }
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Queries with "N".
   
   // Case 23. //
//   clear_hip(f->hip);
   search(f->root, "GTATGNTGATGGACC", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 1);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 24. //
//   clear_hip(f->hip);
   search(f->root, "NTATGCTGATGGACC", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 1);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 25. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGATGGACN", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 1);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 26. //
//   clear_hip(f->hip);
   search(f->root, "NNATGCTGATGGACC", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 27. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGATGGANN", 2);
//   g_assert_cmpint(f->hip->n_hits, ==, 1);
//   g_assert_cmpint(f->hip->hits[0].dist, == , 2);
//   g_assert_cmpstr("GTATGCTGATGGACC", ==, f->hip->hits[0].match);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 28. //
//   clear_hip(f->hip);
   search(f->root, "GTATGCTGNTGGACC", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 2);
//   int dist_28[2] = {1,1};
   for (int i = 0 ; i < 2 ; i++) {
//      g_assert_cmpint(f->hip->hits[i].dist, ==, dist_28[i]);
   }
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Queries with no match.

   // Case 29. //
//   clear_hip(f->hip);
   search(f->root, "GAGTGCAGATGGAGT", 4);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 30. //
//   clear_hip(f->hip);
   search(f->root, "NNNNNNNN", 4);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 31. //
//   clear_hip(f->hip);
   search(f->root, "NNNNNNNN", 7);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 32. //
//   clear_hip(f->hip);
   search(f->root, "ANAA", 0);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 33. //
//   clear_hip(f->hip);
   search(f->root, "GATCGATC", 4);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

   // Case 34. //
//   clear_hip(f->hip);
   search(f->root, "GTAGCTGATGACC", 1);
//   g_assert_cmpint(f->hip->n_hits, ==, 0);
//   g_assert_cmpint(hip_error(f->hip), ==, 0);

}

void
test_mem(
   fixture *f,
   gconstpointer ignore
)
{

//   // Test hip dynamic growth. Build a trie with 1000 sequences
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

//   hip_t *hip = new_hip();
//   g_assert(hip != NULL);
   search(root, "NNNNNNNNNNNNNNNNNNNA", 20);
//   g_assert_cmpint(hip->n_hits, ==, 2049);
//   g_assert_cmpint(hip_error(hip), ==, 0);

   destroy_nodes_downstream_of(root, NULL);
//   destroy_hip(hip);

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

   // Test `search` 1000 times in a perfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      search(f->root, "A", 0);
//      g_assert_cmpint(f->hip->n_hits, ==, 1);
//      g_assert_cmpint(hip_error(f->hip), ==, 0);
//      clear_hip(f->hip);
   }

   // Test `search` in an imperfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(f->root, seq);
      if (node != NULL) node->data = node;
   }
//   clear_hip(f->hip);
   search(f->root, "NNNNNNNNNNNNNNNNNNNN", 20);

   reset_malloc();

}

*/
void
teardown(
   fixture *f,
   gconstpointer test_data
)
{
   destroy_nodes_downstream_of(f->root, NULL);
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
   starcode(inputf, outputf, 3, 1);
   fclose(inputf);

   //inputf = fopen("input_test_file_large.txt", "r");
   //g_assert(inputf != NULL);
   //g_test_timer_start();
   //starcode(inputf, outputf, 3, 1);
   //fclose(inputf);
   //fclose(outputf);
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
   //g_test_add("/trie/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/trie/test_dist_less_than", fixture, NULL, setup, test_dist_less_than, teardown);
   g_test_add("/trie/test_triestack", fixture, NULL, setup, test_triestack, teardown);
   //g_test_add("/test_mem", fixture, NULL, setup, test_mem, teardown);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
