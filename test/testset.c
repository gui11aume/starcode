#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"
#include "starcode.h"

#define SIZE 64

typedef struct {
   trienode *trie;
   hitlist  *hits;
} fixture;

const char *string[] = { "A", "AA",  "AAA", "ATA", "ATT", "AAT",
   "TGA", "TTA", "TAT", "TGC", "GGG", "AAAA", "ACAA", "ANAA", 
   "GTATGCTGATGGACC", "GTATGCTGTGGACC"};

void
setup(
   fixture *f,
   gconstpointer test_data
)
// SYNOPSIS:                                                             
//   Construct a very simple trie for testing purposes.                  
{
   f->hits = new_hitlist();
   f->trie = new_trie();
   for (int i = 0 ; i < 16 ; i++) {
      insert(f->trie, string[i], i+1);
   }
   return;
}

void
test_search(
   fixture *f,
   gconstpointer ignore
)
{

   char b[SIZE];
   // Search the trie designed in `setup` and assert that the
   // number of hits is correct.
   search(f->trie, "AAA", 0, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);
   g_assert_cmpstr("AAA", ==, seq(f->hits->node[0], b, SIZE));

   clear_hitlist(f->hits);
   search(f->trie, "AAA", 1, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 7);

   clear_hitlist(f->hits);
   search(f->trie, "AAA", 2, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 12);

   clear_hitlist(f->hits);
   search(f->trie, "TTT", 1, f->hits);
   g_assert(f->hits->n_hits == 3);

   // Use longer search terms.
   clear_hitlist(f->hits);
   search(f->trie, "GTATGCTGATGGACC", 0, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);

   clear_hitlist(f->hits);
   search(f->trie, "GTATGCTGTGGACC", 0, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);

   clear_hitlist(f->hits);
   search(f->trie, "TATGCTGATGGACC", 1, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, seq(f->hits->node[0], b, SIZE));

   clear_hitlist(f->hits);
   search(f->trie, "TATGCTGATGGACG", 2, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, seq(f->hits->node[0], b, SIZE));

   clear_hitlist(f->hits);
   search(f->trie, "TTATGCTGATGGAC", 2, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, seq(f->hits->node[0], b, SIZE));

   clear_hitlist(f->hits);
   search(f->trie, "TATGCTGATGGAC", 2, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);
   g_assert_cmpstr("GTATGCTGATGGACC", ==, seq(f->hits->node[0], b, SIZE));

   clear_hitlist(f->hits);
   search(f->trie, "GAGTGCAGATGGAGT", 4, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 0);

   clear_hitlist(f->hits);
   search(f->trie, "GAGTGCAGATGGAGT", 5, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 1);

   clear_hitlist(f->hits);
   search(f->trie, "GAGTGCAGATGGAGT", 6, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 2);

   clear_hitlist(f->hits);
   search(f->trie, "GTATGCTGATGGACC", 1, f->hits);
   g_assert_cmpint(f->hits->n_hits, ==, 2);

}


void
test_seq(
   fixture *f,
   gconstpointer ignore
)
{
   // Test the `seq` function, which returns the sequence
   // associated to a trie node.
   char b[8];
   trienode *node = insert(f->trie, "AAATGC", 0);
   g_assert(strcmp(seq(node, b, 8), "AAATGC") == 0);

   node = insert(f->trie, "GYATC", 0);
   g_assert(strcmp(seq(node, b, 8), "GNATC") == 0);

   search(f->trie, "AAA", 0, f->hits);
   g_assert(strcmp(seq(f->hits->node[0], b, 8), "AAA") == 0);
}


void
teardown(
   fixture *f,
   gconstpointer test_data
)
{
   destroy_trie(f->trie);
   destroy_hitlist(f->hits);
}


void
test_run
(void)
{
   FILE *inputf = fopen("input_test_file.txt", "r");
   FILE *outputf = fopen("/dev/null", "w");
   if (inputf == NULL) {
      fprintf(stderr,
            "could not open file 'input_test_file.txt'\n");
      exit (1);
   }
   // Just check that input can be read (test will fail if
   // things go wrong here).
   starcode(inputf, outputf, 3, 0);

   inputf = fopen("input_test_file_large.txt", "r");
   if (inputf == NULL) {
      fprintf(stderr,
            "could not open file 'input_test_file_large.txt'\n");
      exit (1);
   }
   // Run starcode on input file. Used for profiling and memchecking.
   // This runs in about 6-7 minutes on my desktop computer.
   // The input file contains 8,526,061 barcodes, with 210,389
   // unique sequences.
   g_test_timer_start();
   starcode(inputf, outputf, 3, 0);
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
   g_test_add("/trie/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/trie/seq", fixture, NULL, setup, test_seq, teardown);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
