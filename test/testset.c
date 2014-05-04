#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

// -- DECLARATION OF PRIVATE FUNCTIONS FROM trie.c -- //
node_t *insert (node_t *, int, unsigned char);
void init_milestones(node_t*);
node_t *new_trienode(char);
int get_maxtau(node_t*);
int get_height(node_t*);


typedef struct {
   node_t *trie;
} fixture;


// Convenience functions.
int visit(node_t*, char*, int);
char *to_string (node_t *trie, char* s) { visit(trie, s, 0); return s; }
void reset(gstack_t **g) {
   for (int i = 0 ; g[i] != TOWER_TOP ; i++) g[i]->nitems = 0;
}

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
test_base_1
(void)
{
   // Test trie creation and destruction.
   for (char maxtau = 0 ; maxtau < 9 ; maxtau++) {
   for (int height = 0 ; height < M ; height++) {
      node_t *trie = new_trie(maxtau, height);
      g_assert(trie != NULL);
      g_assert(trie->data != NULL);

      g_assert_cmpint(get_maxtau(trie), ==, maxtau);
      g_assert_cmpint(get_height(trie), ==, height);

      // Make sure that 'info' is initialized properly.
      info_t *info = (info_t *) trie->data;
      g_assert(((node_t*) *info->milestones[0]->items) == trie);
      for (int i = 1 ; i < M ; i++) {
         g_assert(info->milestones[i]->items != NULL);
      }
      destroy_trie(trie, NULL);
      trie = NULL;
   }
   }
   return;
}


void
test_base_2
(void)
{
   const char init[19] = {9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8,9};
   // Test node creation and destruction.
   for (char maxtau = 0 ; maxtau < 9 ; maxtau++) {
      node_t *node = new_trienode(maxtau);
      g_assert(node != NULL);
      g_assert(node->cache != NULL);
      g_assert(node->data == NULL);
      g_assert_cmpint(node->path, ==, 0);
      for (int i = 0 ; i < 6 ; i++) {
         g_assert(node->child[i] == NULL);
      }
      for (int i = 0 ; i < 2*maxtau + 3 ; i++) {
         g_assert(node->cache[i] == init[i+(8-maxtau)]);
      }
      free(node);
   }
   return;
}


void
test_base_3
(void)
{
   node_t *trie = new_trie(3, 20);
   g_assert(trie != NULL);

   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert(trie, i, 3);
      g_assert(node != NULL);
      g_assert(trie->child[i] == node);
      g_assert_cmpint(node->path, ==, i);
   }
   destroy_trie(trie, NULL);
   return;
}


void
test_base_4
(void)
{
   node_t *trie = new_trie(3, 20);
   g_assert(trie != NULL);

   node_t *node = insert_string(trie, "AAAAAAAAAAAAAAAAAAAA");
   g_assert(node != NULL);
   g_assert_cmpint(21, ==, count_nodes(trie));

   destroy_trie(trie, NULL);
   return;

}


void
test_base_5
(void)
{
   gstack_t *gstack = new_gstack();
   g_assert(gstack != NULL);
   g_assert(gstack->items != NULL);
   g_assert_cmpint(gstack->nslots, ==, STACK_INIT_SIZE);
   g_assert_cmpint(gstack->nitems, ==, 0);

   node_t *node = new_trienode(3);
   push(node, &gstack);
   g_assert(node == (node_t *) gstack->items[0]);
   g_assert_cmpint(gstack->nslots, ==, STACK_INIT_SIZE);
   g_assert_cmpint(gstack->nitems, ==, 1);

   free(node);
   free(gstack);
   return;
}


void
test_base_6
(void)
{
   gstack_t **tower = new_tower(1);
   g_assert(tower != NULL);
   g_assert_cmpint(tower[0]->nslots, ==, STACK_INIT_SIZE);
   g_assert_cmpint(tower[0]->nitems, ==, 0);
   return;
}


void
test_search(
   fixture *f,
   gconstpointer ignore
)
{
//   hstack_t *hstack = new_hstack();
//   g_assert(hstack != NULL);
   gstack_t **hits = new_tower(4);

   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
//   g_assert_cmpint(hstack->pos, ==, 6);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 4);
   g_assert_cmpint(hits[2]->nitems, ==, 1);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   /*
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, &hstack, 18, 3);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, "AAAGAAAAAAAAAAAAAATA", 3, &hstack, 3, 15);
   g_assert_cmpint(hstack->pos, ==, 5);

   reset(hstack);
   search(f->trie, "AAAGAAAAAAAAAAAGACTG", 3, &hstack, 15, 15);
   g_assert_cmpint(hstack->pos, ==, 0);

   reset(hstack);
   search(f->trie, "AAAGAAAAAAAAAAAAAAAA", 3, &hstack, 15, 0);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAA", 3, &hstack, 0, 19);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAG", 3, &hstack, 19, 0);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, " AAAAAAAAAAAAAAAAAAA", 3, &hstack, 0, 19);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, " AAAAAAAAAAAAAAAAAAT", 3, &hstack, 19, 0);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, &hstack, 0, 0);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, " TGCTAGGGTACTCGATAAC", 1, &hstack, 0, 20);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, "NAAAAAAAAAAAAAAAAAAN", 2, &hstack, 0, 1);
   g_assert_cmpint(hstack->pos, ==, 5);

   reset(hstack);
   search(f->trie, "NNAAAAAAAAAAAAAAAANN", 3, &hstack, 1, 0);
   g_assert_cmpint(hstack->pos, ==, 0);

   reset(hstack);
   search(f->trie, "AAAAAAAAAANAAAAAAAAA", 0, &hstack, 0, 0);
   g_assert_cmpint(hstack->pos, ==, 0);

   reset(hstack);
   search(f->trie, "NNNAGACTTTTCCAGGGTAT", 3, &hstack, 0, 0);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, "GGGAGACTTTTCCAGGGNNN", 3, &hstack, 0, 17);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, "GGGAGACTTTTCCAGGG   ", 3, &hstack, 17, 0);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, "   AGACTTTTCCAGGGTAT", 3, &hstack, 0, 3);
   g_assert_cmpint(hstack->pos, ==, 1);

   reset(hstack);
   search(f->trie, "                   N", 1, &hstack, 3, 19);
   g_assert_cmpint(hstack->pos, ==, 3);

   reset(hstack);
   search(f->trie, "                    ", 1, &hstack, 19, 0);
   g_assert_cmpint(hstack->pos, ==, 3);

   // Caution here: the hit is present but the initial
   // search conditions are wrong.
   reset(hstack);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, &hstack, 20, 1);
   g_assert_cmpint(hstack->pos, ==, 0);

   // Repeat first test cases.
   reset(hstack);
   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, &hstack, 0, 18);
   g_assert_cmpint(hstack->pos, ==, 6);

   reset(hstack);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, &hstack, 18, 18);
   g_assert_cmpint(hstack->pos, ==, 6);
   */

//   free(hstack);
   destroy_tower(hits);
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
   gstack_t **hits = new_tower(4);
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
   err = search(f->trie, "AAAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 0);
   g_assert_cmpint(err, ==, 65);
   unredirect_sderr();
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: query longer than allowed max\n");
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   redirect_stderr_to(ERROR_BUFFER);
   err = search(f->trie, " TGCTAGGGTACTCGATAAC", 4, hits, 0, 0);
   unredirect_sderr();
   g_assert_cmpint(err, ==, 59);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: requested tau greater than 'maxtau'\n");
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

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
   gstack_t *gstack_fail = new_gstack();
   reset_alloc();
   unredirect_sderr();
   g_assert(gstack_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create gstack\n");

   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   gstack_t **tower_fail = new_tower(1);
   reset_alloc();
   unredirect_sderr();
   g_assert(tower_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
      "error: could not create tower\n");

   destroy_tower(hits);
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
   // Test hstack dynamic growth. Build a trie with 1000 sequences
   // and search with maxdist of 20 (should return all the sequences).
   char seq[9];
   seq[8] = '\0';

   node_t *trie = new_trie(8, 8);
   g_assert(trie != NULL);

   gstack_t **hits = new_tower(9);
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
   err = search(trie, "NNNNNNNN", 8, hits, 0, 8);
   unredirect_sderr();
   reset_alloc();

   g_assert_cmpint(err, >, 0);
   g_assert_cmpint(hits[8]->nitems, >, hits[8]->nslots);

   reset(hits);

   // Do it again without failure.
   err = search(trie, "NNNNNNNN", 8, hits, 0, 8);
   g_assert_cmpint(err, ==, 0);

   destroy_trie(trie, NULL);
   destroy_tower(hits);
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
   // The variable 'trie' and 'hstack' are initialized
   // without 'malloc()' failure.
   node_t *trie = new_trie(3, 20);
   g_assert(trie != NULL);

   gstack_t **hits = new_tower(4);
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
      err = search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 20);
      g_assert_cmpint(err, == , 0);
      g_assert_cmpint(hits[0]->nitems, ==, 1);
      g_assert_cmpint(hits[1]->nitems, ==, 4);
      g_assert_cmpint(hits[2]->nitems, ==, 1);
      g_assert_cmpint(hits[3]->nitems, ==, 0);
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
      search(trie, seq, 3, hits, 0, 20);
   }
   unredirect_sderr();
   reset_alloc();
   destroy_tower(hits);
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
   gstack_t *gstack;
   gstack_t **tower;

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
         free(trienode);
         trie = NULL;
      }
   }


   // Test 'new_gstack()'.
   for (int i = 0 ; i < 1000 ; i++) {
      gstack = new_gstack();
      if (gstack == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         free(gstack);
         gstack = NULL;
      }
   }

   // Test 'new_tower()'.
   for (int i = 0 ; i < 1000 ; i++) {
      tower = new_tower(3);
      if (tower == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
      }
      else {
         destroy_tower(tower);
         tower = NULL;
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
   g_test_add_func("/base/1", test_base_1);
   g_test_add_func("/base/2", test_base_2);
   g_test_add_func("/base/3", test_base_3);
   g_test_add_func("/base/4", test_base_4);
   g_test_add_func("/base/5", test_base_5);
   g_test_add_func("/base/6", test_base_6);
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
