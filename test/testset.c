#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

#define NSTRINGS 22

typedef struct {
   node_t *trie;
} fixture;


const char *string[NSTRINGS] = {
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


// Convenience function.
void reset(narray_t *h) { h->pos = 0; h->err = 0; }


// Error message handling functions.
char error_buffer[1024];
int backup_file_descriptor;

void
redirect_stderr_to
(char buffer[])
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
   dup2(backup_file_descriptor, STDERR_FILENO);
   setvbuf(stderr, NULL, _IONBF, 0);
}


// String representation of the trie as a depth first search
// traversal. The stars indicate the a series of with no further
// children. I also use indentation to ease the reading.
char repr[1024];
char trierepr[] =
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

void
strie(
   node_t *node,
   int restart
)
// String representation of the trie.
{
   static int j;
   if (restart) j = 0;
   for (int i = 0 ; i < 6 ; i++) {
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
   if (f->trie == NULL || f->trie->data == NULL) {
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

   int err = 0 ;
   narray_t *hits = new_narray();
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

   // Check error messages.
   reset(hits);
   redirect_stderr_to(error_buffer);
   err = search(f->trie, "AAAAAAAAAAAAAAAAAAAAA", 3, &hits, 1, 0);
   g_assert_cmpint(err, ==, 91);
   unredirect_sderr();
   g_assert_cmpstr(error_buffer, ==,
         "query longer than allowed max\n");
   g_assert_cmpint(hits->pos, ==, 0);

   reset(hits);
   redirect_stderr_to(error_buffer);
   err = search(f->trie, " TGCTAGGGTACTCGATAAC", 4, &hits, 0, 0);
   unredirect_sderr();
   g_assert_cmpint(err, ==, 85);
   g_assert_cmpstr(error_buffer, ==,
         "requested tau greater than 'maxtau'\n");
   g_assert_cmpint(hits->pos, ==, 0);


   // Repeat first test cases.
   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 18);
   g_assert_cmpint(hits->pos, ==, 6);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, &hits, 18, 18);
   g_assert_cmpint(hits->pos, ==, 6);



   free(hits);

}

void
test_mem(
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
   // Insert 2049 random sequences.
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 8 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(trie, seq);
      g_assert(node != NULL);
      node->data = node;
   }

   // The node array 'hits' is properly initialized.
   narray_t *hits = new_narray();
   g_assert(hits != NULL);

   reset(hits);
   search(trie, "NNNNNNNN", 8, &hits, 0, 0);
   g_assert_cmpint(hits->pos, >, 0);
   g_assert_cmpint(check_trie_error_and_reset(), ==, 0);

   // Do it again with a memory failure.
   free(hits);
   hits = new_narray();

   set_alloc_failure_rate_to(1);
   search(trie, "NNNNNNNN", 8, &hits, 0, 0);
   reset_alloc();
   g_assert_cmpint(hits->err, ==, 1);

   reset(hits);
   destroy_trie(trie, NULL);

   // Construct 1000 tries with a `malloc` failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_alloc_failure_rate_to(0.01);

   for (int i = 0 ; i < 1000 ; i++) {
      node_t *trie = new_trie(3, 20);
      if (trie == NULL) {
         // Make sure errors are reported.
         g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         continue;
      }
      for (int j = 0 ; j < NSTRINGS ; j++) {
         node_t *node = insert_string(trie, string[j]);
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

   // Test 'search()' 1000 times in a perfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      reset(hits);
      search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hits, 0, 20);
      if (hits->pos != 6) {
         g_assert_cmpint(hits->err, == , 1);
      }
   }

   check_trie_error_and_reset();

   // Test 'search()' in an imperfectly built trie.
   for (int i = 0 ; i < 2048 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      node_t *node = insert_string(f->trie, seq);
      if (node == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         node->data = node;
      }
   }

   for (int i = 0 ; i < 1000 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      reset(hits);
      search(f->trie, seq, 3, &hits, 0, 20);
   }

   // Calls to 'search()' do not create errors. 'realloc()' is called
   // when 'hits' is full, which won't happen with a sparse trie.
   g_assert_cmpint(check_trie_error_and_reset(), == , 0);

   reset_alloc();
   free(hits);

}


void
test_count
(
   fixture *f,
   gconstpointer ignore
)
{
   int n_nodes = 1;
   // Count nodes from the string representation of the trie,
   // not forgetting the root which is not represented.
   for (int i = 0 ; i < strlen(trierepr) ; i++) {
      n_nodes += trierepr[i] != '*';
   }
   g_assert_cmpint(n_nodes, ==, count_nodes(f->trie));
}


void
test_compress
(
   fixture *f,
   gconstpointer ignore
)
{
   // Set the 'data' of every node to its own address (except root).
   narray_t *stack = new_narray();
   push(f->trie, &stack);
   while (stack->pos > 0) {
      node_t *node = stack->nodes[--stack->pos];
      for (int i = 0 ; i < 6 ; i++) {
         node_t *child = node->child[i];
         if (child != NULL) {
            child->data = child;
            push(child, &stack);
         }
      }
   }

   a_node_t *a_trie = compress_trie(f->trie);
   int n_nodes = count_nodes(f->trie);

   // Check the trie structure in the array trie.
   for (int i = 1 ; i < n_nodes ; i++) {
      a_node_t *a_node = &a_trie[i];
      node_t *node = (node_t *) a_node->data;
      for (int j = 0 ; j < 6 ; j++) {
         // If no child in the array trie, no child in the trie.
         if (!a_node->child[j]) g_assert(node->child[j] == NULL);
         // Otherwise, make sure that children are the same.
         else {
            node_t *a = (node_t *) a_trie[a_node->child[j]].data;
            node_t *b = (node_t *) node->child[j]->data;
            g_assert(a == b);
         }
      }
   }
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
   starcode(inputf, outputf, 3, 0, 1);
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
   backup_file_descriptor = dup(STDERR_FILENO);

   g_test_init(&argc, &argv, NULL);
   g_test_add("/count", fixture, NULL, setup, test_count, teardown);
   g_test_add("/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/mem", fixture, NULL, setup, test_mem, teardown);
   g_test_add("/compress", fixture, NULL, setup, test_compress, teardown);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
