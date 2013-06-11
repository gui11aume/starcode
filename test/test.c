#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"
#include "starcode.h"

typedef struct {
   trienode *trie;
   hitlist  *hits;
} fixture;

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
   insert(f->trie, "A", 1);
   insert(f->trie, "AA", 2);
   insert(f->trie, "AAA", 4);
   insert(f->trie, "ATA", 5);
   insert(f->trie, "AAT", 6);
   insert(f->trie, "TGA", 7);
   insert(f->trie, "TGC", 8);
   insert(f->trie, "AAAA", 9);
   insert(f->trie, "ACAA", 10);
   insert(f->trie, "ANAA", 11);
   return;
}

void
test_search(
   fixture *f,
   gconstpointer ignore
)
{
   // Search the trie designed in `setup` and assert that the
   // number of hits is correct. Always search the same sequence
   // (AAA) but with different number of allowed mismatches.
   search(f->trie, "AAA", 0, f->hits);
   g_assert(f->hits->n_hits == 1);

   clear_hitlist(f->hits);
   search(f->trie, "AAA", 1, f->hits);
   g_assert(f->hits->n_hits == 3);

   clear_hitlist(f->hits);
   search(f->trie, "AAA", 2, f->hits);
   g_assert(f->hits->n_hits == 4);
}


void
test_seq(
   fixture *f,
   gconstpointer ignore
)
{
   // Test the `seq` function, which returns the sequence
   // associated to a trie node.
   char buffer[8];
   trienode *node = insert(f->trie, "AAATGC", 0);
   g_assert(strcmp(seq(node, buffer, 8), "AAATGC") == 0);

   node = insert(f->trie, "GYATC", 0);
   g_assert(strcmp(seq(node, buffer, 8), "GNATC") == 0);

   search(f->trie, "AAA", 0, f->hits);
   g_assert(strcmp(seq(f->hits->node[0], buffer, 8), "AAA") == 0);
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
   FILE *inputf = fopen("input_test_file", "r");
   FILE *outputf = fopen("/dev/null", "w");
   // Run starcode on input file. Used for profiling and memchecking.
   // This runs in about 30 seconds on my desktop computer.
   // The input file contains 8,526,061 barcodes, with 210,389
   // unique sequences.
   starcode(inputf, outputf, 0);
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
   g_test_add_func("/starcode/run", test_run);
   return g_test_run();
}
