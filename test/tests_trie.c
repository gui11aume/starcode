#include "unittest.h"
#include "trie.c"

// Globals //
int LEAF_NODE;

// Convenience functions.
void reset_gstack(gstack_t **g) {
   for (int i = 0 ; g[i] != TOWER_TOP ; i++) g[i]->nitems = 0;
}

char *
render
(
   node_t * node,
   char   * string
)
{
   for (int i = 0 ; i < 6 ; i++) {
      void * child = node->child[i];
      if (child == NULL) continue;
      *string++ = untranslate[i];
      if (child == &LEAF_NODE) continue;
      string = render(node->child[i], string);
   }
   return string;
}


//  Test setup function  //

trie_t *
setup
(void)
//   Construct a very simple trie for testing purposes.                  
{

   const char *string[24] = {
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
   "GGGAGAC----CCAGGGTAT",
   "GGGAGAC----GCAGGGTAT",
   };

   trie_t * trie = new_trie(20);
   if (trie == NULL) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      abort();
   }

   for (int i = 0 ; i < 24 ; i++) {
      void **data = insert_string(trie, string[i]);
      // Set node data to self (non 'NULL').
      if (data != NULL) *data = &LEAF_NODE;
      else {
         fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
         abort();
      }
   }

   char trie_rendering[] =
   "NNAAAAAAAAAAAAAAAANN"
    "AAAAAAAAAAAAAAAAAAN"
   "AAAAAAAAAANAAAAAAAAA"
             "AAAAAAAAAA"
                      "T"
    "TGCTAGGGTACTCGATAAC"
   "CATACAGTATTCGACTAAGG"
    "GAGGCGTATAGTGTTTCCA"
   "GGATTAGATCACCGCTTTCG"
     "GAGACNNNNCCAGGGTAT"
              "GCAGGGTAT"
          "TTTTCCAGGGTAT"
   "TAAAAAAAAAAAAAAAAAAA"
    "CATTGTGATAGCTCGTAAC"
    "GGAGATGATGAAGAAGACC"
    "TCGGAGCGACTAATATAGG"
     "GGTATATGTCATAGAAAT"
   " AAAAAAAAAAAAAAAAAAA"
    "GGATATCAAGGGTTACTAG"
    "           NNNNNNNN"
               "AAAAAAAA"
               "       N"
                      "A"
                      " ";

   // Make sure the trie has the correct structure.
   char * trie_buff = calloc(512, sizeof(char));
   if (trie_buff == NULL) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      abort();
   }
   render(trie->root, trie_buff);
   if (strcmp(trie_rendering, trie_buff) != 0) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      abort();
   }
   //if (count_nodes(trie) != 338) {
   if (count_nodes(trie) != 360) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      abort();
   }

   free(trie_buff);
   return trie;

}

void
teardown
(
   trie_t * trie
)
{
   destroy_trie(trie, DESTROY_NODES_YES, NULL);
}



void
test_base_1
(void)
// Test node creation and destruction.
{

   const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
   node_t *node = new_trienode();
   // Check that creation succeeded.
   test_assert_critical(node != NULL);
   // Check initialization.
   test_assert(node->path == 0);
   for (int i = 0 ; i < 6 ; i++) {
      test_assert(node->child[i] == NULL);
   }
   for (int i = 0 ; i < 2*TAU+1 ; i++) {
      test_assert(node->cache[i] == cache[i]);
   }
   
   free(node);

}

void
test_base_2
(void)
// Test 'insert()'.
{
   node_t *root = new_trienode();
   test_assert_critical(root != NULL);

   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert(root, i);
      test_assert_critical(node != NULL);
      for (int j = 0 ; j < 6 ; j++) {
         test_assert(node->child[j] == NULL);
      }
      const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
      for (int i = 0 ; i < 2*TAU+1 ; i++) {
         test_assert(node->cache[i] == cache[i]);
      }
      test_assert(node->path == i);
      test_assert(root->child[i] == node);
   }

   // Destroy manually.
   for (int i = 0 ; i < 6 ; i++) free(root->child[i]);
   free(root);

}


void
test_base_3
(void)
// Test 'insert_wo_malloc()'.
{
   node_t *root = new_trienode();
   test_assert_critical(root != NULL);

   node_t *nodes = malloc(6 * sizeof(node_t));
   if (nodes == NULL) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert_wo_malloc(root, i, nodes+i);
      test_assert_critical(node != NULL);
      test_assert(node == nodes+i);
      for (int j = 0 ; j < 6 ; j++) {
         test_assert(node->child[j] == NULL);
      }
      const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
      for (int i = 0 ; i < 2*TAU+1 ; i++) {
         test_assert(node->cache[i] == cache[i]);
      }
      test_assert(node->path == i);
      test_assert(root->child[i] == node);
   }

   // Try to overwrite nodes (should not succeed);
   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert_wo_malloc(root, i, 0x0);
      test_assert(node == NULL);
   }

   free(root);
   free(nodes);

}


void
test_base_4
(void)
// Test trie creation and destruction.
{

   srand48(123);

   for (int height = 1 ; height < M ; height++) {
      trie_t *trie = new_trie(height);
      test_assert_critical(trie != NULL);
      test_assert_critical(trie->root != NULL);
      test_assert_critical(trie->info != NULL);

      test_assert(get_height(trie) == height);

      // Make sure that 'info' is initialized properly.
      info_t *info = trie->info;
      test_assert(((node_t*) *info->pebbles[0]->items) == trie->root);
      for (int i = 1 ; i < M ; i++) {
         test_assert_critical(info->pebbles[i]->items != NULL);
      }

      // Insert 20 random sequences.
      for (int i = 0 ; i < 20 ; i++) {
         char seq[M] = {0};
         for (int j = 0 ; j < height ; j++) {
            seq[j] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string(trie, seq);
         test_assert(data != NULL);
         *data = data;
      }
      destroy_trie(trie, DESTROY_NODES_YES, NULL);
      trie = NULL;
   }

   for (int height = 1 ; height < M ; height++) {
      trie_t *trie = new_trie(height);
      test_assert_critical(trie != NULL);
      test_assert_critical(trie->root != NULL);
      test_assert_critical(trie->info != NULL);

      test_assert(get_height(trie) == height);

      // Make sure that 'info' is initialized properly.
      info_t *info = trie->info;
      test_assert(((node_t*) *info->pebbles[0]->items) == trie->root);
      for (int i = 1 ; i < M ; i++) {
         test_assert_critical(info->pebbles[i]->items != NULL);
      }

      // Insert 20 random sequences without malloc.
      node_t *nodes = malloc(20*height * sizeof(node_t));
      if (nodes == NULL) {
         fprintf(stderr, "unittest error: %s:%d", __FILE__, __LINE__);
         exit(EXIT_FAILURE);
      }

      node_t *pos = nodes;
      for (int i = 0 ; i < 20 ; i++) {
         char seq[M] = {0};
         for (int j = 0 ; j < height ; j++) {
            seq[j] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string_wo_malloc(trie, seq, &pos);
         test_assert(data != NULL);
         *data = data;
      }
      destroy_trie(trie, DESTROY_NODES_NO, NULL);
      free(nodes);
      trie = NULL;
   }
   
}


void
test_base_5
(void)
// Test 'insert_string()'.
{

   trie_t *trie = new_trie(20);

   void **data = insert_string(trie, "AAAAAAAAAAAAAAAAAAAA");
   test_assert(data != NULL);
   *data = data;
   test_assert(21 == count_nodes(trie));

   destroy_trie(trie, DESTROY_NODES_YES, NULL);

}


void
test_base_6
(void)
// Test 'insert_string_wo_malloc()'.
{
   trie_t *trie = new_trie(20);
   test_assert_critical(trie != NULL);

   node_t *nodes = malloc(19 * sizeof(node_t));
   if (nodes == NULL) {
      fprintf(stderr, "unittest error (%s:%d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
   node_t *pos = nodes;
   void **data = 
      insert_string_wo_malloc(trie, "AAAAAAAAAAAAAAAAAAAA", &pos);
   test_assert(data != NULL);
   *data = data;
   test_assert(21 == count_nodes(trie));
   // Check that the pointer has been incremented by 19 positions.
   test_assert(19 == (pos - nodes));

   // Cache at initialization.
   const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
   // Successive 'paths' members of a line of A in the trie.
   const int paths[20] = {0,1,17,273,4369,69905,1118481,17895697,
      286331153,286331153,286331153,286331153,286331153,286331153,
      286331153,286331153,286331153,286331153,286331153,286331153};

   // Check the integrity of the nodes.
   node_t *node = trie->root;
   for (int i = 0 ; i < 20 ; i++) {
      test_assert_critical(node != NULL);
      test_assert(node->cache != NULL);
      test_assert(node->path == paths[i]);
      test_assert(node->child[0] == NULL);
      test_assert(node->child[1] != NULL);
      for (int j = 2 ; j < 6 ; j++) {
         test_assert(node->child[j] == NULL);
      }
      for (int j = 0 ; j < 2*TAU+1 ; j++) {
         test_assert(node->cache[j] == cache[j]);
      }
      node = node->child[1];
   }

   destroy_trie(trie, DESTROY_NODES_NO, NULL);
   free(nodes);

}


void
test_base_7
(void)
// Test 'new_gstack()' and 'push()'.
{
   gstack_t *gstack = new_gstack();
   test_assert_critical(gstack != NULL);
   test_assert(gstack->items != NULL);
   test_assert(gstack->nslots == GSTACK_INIT_SIZE);
   test_assert(gstack->nitems == 0);

   node_t *node = new_trienode();
   push(node, &gstack);
   test_assert(node == (node_t *) gstack->items[0]);
   test_assert(gstack->nslots == GSTACK_INIT_SIZE);
   test_assert(gstack->nitems == 1);

   free(node);
   free(gstack);
   
}


void
test_base_8
(void)
// Test 'new_tower()'.
{

   for (int i = 1 ; i < 100 ; i++) {
      gstack_t **tower = new_tower(i);
      test_assert_critical(tower != NULL);
      for (int j = 0 ; j < i ; j++) {
         test_assert(tower[j]->nslots == GSTACK_INIT_SIZE);
         test_assert(tower[j]->nitems == 0);
      }
      test_assert(tower[i] == TOWER_TOP);
      destroy_tower(tower);
      tower = NULL;
   }
   
}


void
test_errmsg
(void)
{

   int err;

   // Check error messages in 'insert_string()'.
   redirect_stderr();
   trie_t * trie = new_trie(0);
   unredirect_stderr();
   test_assert(trie == NULL);
   test_assert_stderr("error: the minimum trie height is 1\n");

   trie = setup();

   // Check error messages in 'insert_string()'.
   char too_long_string[M+2];
   for (int i = 0 ; i < M+1 ; i++) too_long_string[i] = 'A';
   too_long_string[M+1] = '\0';
   redirect_stderr();
   void **not_inserted = insert_string(trie, too_long_string);
   unredirect_stderr();

   char string[1024];
   sprintf(string, "error: can only insert string of length %d\n",
         get_height(trie));
   test_assert(not_inserted == NULL);
   test_assert_stderr(string);

   // Check error messages in 'insert_string_wo_malloc()'.
   node_t * dummy;
   redirect_stderr();
   not_inserted = insert_string_wo_malloc(trie, too_long_string, &dummy);
   unredirect_stderr();
   test_assert(not_inserted == NULL);
   test_assert_stderr(string);

   // Check error messages in 'search()'.
   gstack_t **hits = new_tower(4);
   test_assert_critical(hits != NULL);

   redirect_stderr();
   err = search(trie, "AAAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 0);
   unredirect_stderr();
   test_assert(err > 0);
   test_assert_stderr("error: query longer than allowed max\n");
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   redirect_stderr();
   err = search(trie, " TGCTAGGGTACTCGATAAC", 9, hits, 0, 0);
   unredirect_stderr();
   test_assert(err > 0);
   test_assert_stderr("error: requested tau greater than 8\n");
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   // Force 'malloc()' to fail and check error messages of constructors.
   redirect_stderr();
   set_alloc_failure_rate_to(1);
   trie_t *trie_fail = new_trie(20);
   reset_alloc();
   unredirect_stderr();
   test_assert(trie_fail == NULL);
   test_assert(check_trie_error_and_reset() >  0);
   test_assert_stderr("error: could not create trie\n");

   redirect_stderr();
   set_alloc_failure_rate_to(1);
   node_t *trienode_fail = new_trienode();
   reset_alloc();
   unredirect_stderr();
   test_assert(trienode_fail == NULL);
   test_assert(check_trie_error_and_reset() >  0);
   test_assert_stderr("error: could not create trie node\n");

   redirect_stderr();
   set_alloc_failure_rate_to(1);
   gstack_t *gstack_fail = new_gstack();
   reset_alloc();
   unredirect_stderr();
   test_assert(gstack_fail == NULL);
   test_assert(check_trie_error_and_reset() >  0);
   test_assert_stderr("error: could not create gstack\n");

   redirect_stderr();
   set_alloc_failure_rate_to(1);
   gstack_t **tower_fail = new_tower(1);
   reset_alloc();
   unredirect_stderr();
   test_assert(tower_fail == NULL);
   test_assert(check_trie_error_and_reset() >  0);
   test_assert_stderr("error: could not create tower\n");

   destroy_tower(hits);
   teardown(trie);

}


void
test_search
(void)
{

   int err;

   trie_t * trie = setup();
   gstack_t **hits = new_tower(4);
   test_assert_critical(hits != NULL);

   // The first series of test has been worked out manually. //
   
   err = search(trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 4);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAAAAAAAAAATA", 3, hits, 18, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 3);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "AAAGAAAAAAAAAAAAAATA", 3, hits, 3, 15);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 2);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, "AAAGAAAAAAAAAAAGACTG", 3, hits, 15, 15);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "AAAGAAAAAAAAAAAAAAAA", 3, hits, 15, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 3);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 19);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 3);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "TAAAAAAAAAAAAAAAAAAG", 3, hits, 19, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 1);
   test_assert(hits[2]->nitems == 4);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAAAAAAAAAAAAAAAAAA", 3, hits, 0, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 4);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "   ATGCTAGGGTACTCGAT", 3, hits, 0, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAAAAAAAAAAAAAAAAAT", 3, hits, 1, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 4);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "ATGCTAGGGTACTCGATAAC", 0, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " TGCTAGGGTACTCGATAAC", 1, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 1);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "NAAAAAAAAAAAAAAAAAAN", 2, hits, 0, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 5);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "NNAAAAAAAAAAAAAAAANN", 3, hits, 1, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAANAAAAAAAAA", 0, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "NNNAGACTTTTCCAGGGTAT", 3, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "GGGAGACTTTTCCAGGGNNN", 3, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "   GGGAGACTTTTCCAGGG", 3, hits, 0, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "   AGACTTTTCCAGGGTAT", 3, hits, 3, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "                   N", 1, hits, 3, 19);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 3);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "                    ", 1, hits, 19, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   // Caution here: the hit is present but the initial
   // search conditions are wrong.
   reset_gstack(hits);
   search(trie, "ATGCTAGGGTACTCGATAAC", 0, hits, 20, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   // Repeat first test cases.
   reset_gstack(hits);
   search(trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 4);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAAAAAAAAAATA", 3, hits, 18, 18);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 2);
   test_assert(hits[2]->nitems == 3);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "GGGAGAC----CCAGGGTAT", 3, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 1);
   test_assert(hits[1]->nitems == 1);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "GGGATTT----GCAGGGTAT", 3, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   // The tests below were randomly generated by a Python script. //

   reset_gstack(hits);
   search(trie, "             CAAAAAT", 3, hits, 0, 12);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "            AAAAGATA", 3, hits, 12, 14);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "            AAGAAACC", 3, hits, 14, 13);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "            ATAANTAA", 3, hits, 13, 12);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "            TCAANAAA", 3, hits, 12, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "   AAAAAAAAAAAAAAAGA", 3, hits, 3, 16);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "   AAAAAAAAAAAAANAAA", 3, hits, 16, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "   GGATTCAAGGTTACTAG", 3, hits, 3, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "  AAAAAAAAAAAAAAACAA", 3, hits, 2, 13);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 5);

   reset_gstack(hits);
   search(trie, "  AAAAAAAAAAANAAAAAT", 3, hits, 13, 5);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 2);

   reset_gstack(hits);
   search(trie, "  AAATAAAANAAAAAAAAA", 3, hits, 5, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "  GGANTCAAGGGTTACTAG", 3, hits, 2, 5);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "  GGATTAGATCCCGCTTTG", 3, hits, 5, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "  TGNGATGAGAAGAAGACC", 3, hits, 2, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "  TTCGGGCGACNATATAGG", 3, hits, 3, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAAAANATAANAAAAAAAA", 3, hits, 1, 5);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAAATAAAAAAAAACAAAA", 3, hits, 5, 15);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 4);

   reset_gstack(hits);
   search(trie, " AAAATAAAAAAAAATAAAA", 3, hits, 15, 4);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 4);

   reset_gstack(hits);
   search(trie, " AAACAAAAANAAAAAAGAA", 3, hits, 4, 4);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAATAAAAANAAAAAAANA", 3, hits, 4, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " AAGCTAGGGTACTCGATGC", 3, hits, 3, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " ANAAAAAAAAAAAGAAANA", 3, hits, 2, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " ATGCTAGGGACTCTATAAC", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " CAACAAAAAAAAAAAAAAN", 3, hits, 1, 6);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " CAACAGTCTTCGACTAANG", 3, hits, 6, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " GAAAAGAAAAAAAAAAAAA", 3, hits, 1, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 5);

   reset_gstack(hits);
   search(trie, " GGAGACTTCTCCAGGGTAG", 3, hits, 2, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " GGGGATCATGGGTTACTAG", 3, hits, 3, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " GTGCTAGGTACTCGATAAC", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " NCATTGTATAGCCCGTAAC", 3, hits, 1, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " TAAGAAAAAAAAAAAAAAT", 3, hits, 1, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, " TCATTATTATAGCTCGTAC", 3, hits, 2, 6);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " TCATTGTGATGCTCGTATC", 3, hits, 6, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " TGATATAAAGGGTTTCTAG", 3, hits, 2, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " TTGCTAGGGTACTCGATAC", 3, hits, 2, 9);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " TTGCTAGGTANTCGATAAC", 3, hits, 9, 4);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, " TTGTATATGTCNTAGAAAT", 3, hits, 4, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, " TTTGTATGTGTCATAGAAA", 3, hits, 3, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAAGGAAANAAAT", 3, hits, 0, 10);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAANAAAATAAAA", 3, hits, 10, 9);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 3);
   test_assert(hits[3]->nitems == 2);

   reset_gstack(hits);
   search(trie, "AAAAAAAAAGAACAAACAAA", 3, hits, 9, 5);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 2);

   reset_gstack(hits);
   search(trie, "AAAAATANAAAAAAAAAAAT", 3, hits, 5, 4);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 2);

   reset_gstack(hits);
   search(trie, "AAAACAAAAAACACAAAAAA", 3, hits, 4, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 2);

   reset_gstack(hits);
   search(trie, "AAAGNAAAAANAAAAAAAAA", 3, hits, 3, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, "AAANAACAAAAAAAAAAAAA", 3, hits, 3, 3);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 2);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, "AAATAAAAAAAAGAAAAAAA", 3, hits, 3, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 2);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, "AACCAAAAAAAAAAANAAAT", 3, hits, 2, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "AAGANAAAAANAAAAAAAAA", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 3);

   reset_gstack(hits);
   search(trie, "ATGCTAGGGTACTCGATAAN", 3, hits, 1, 6);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 1);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "ATGCTANGATAGTCGATAAC", 3, hits, 6, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "CANACAGTATACGACTANGG", 3, hits, 0, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "CATACAGTATACGCCTAAGA", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "CGAGGCGTAGAGTATTTCGA", 3, hits, 1, 8);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "CGAGGCGTTCAGTGTTTGCA", 3, hits, 8, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "GGATTAGTTCACCGCTATCG", 3, hits, 0, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "TAAAAAAANAAAAACACAAA", 3, hits, 0, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TAGAAAAAAAANAAAAAAAG", 3, hits, 2, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TANTTGCGATAGCTCGTAAC", 3, hits, 2, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TATAAAAAAAAAAAANCAAA", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TCATTGTGATAGCANGTAGC", 3, hits, 1, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TNCGNAGCGACTAATATGGG", 3, hits, 1, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TNGAGATGATGGAGAANACC", 3, hits, 2, 1);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 0);
   test_assert(hits[3]->nitems == 1);

   reset_gstack(hits);
   search(trie, "TTCTGANCGACTAATATAGG", 3, hits, 1, 2);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   reset_gstack(hits);
   search(trie, "TTGGTNTTTGTCATAGAAAT", 3, hits, 2, 0);
   test_assert(err == 0);
   test_assert(check_trie_error_and_reset() == 0);
   test_assert(hits[0]->nitems == 0);
   test_assert(hits[1]->nitems == 0);
   test_assert(hits[2]->nitems == 1);
   test_assert(hits[3]->nitems == 0);

   // End random tests. //

   destroy_tower(hits);
   teardown(trie);

}


void
test_mem_1
(void)
{

   srand48(123);
   int err;

   // Test hstack dynamic growth. Build a trie with 1000 sequences
   // and search with maxdist of 20 (should return all the sequences).
   char seq[9];
   seq[8] = '\0';

   trie_t *trie = new_trie(8);
   test_assert_critical(trie != NULL);

   gstack_t **hits = new_tower(9);
   test_assert_critical(hits != NULL);
 
   // Insert 2049 random sequences.
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 8 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      void **data = insert_string(trie, seq);
      test_assert(data != NULL);
      *data = data;
   }

   // Search with failure in 'malloc()'.
   set_alloc_failure_rate_to(1);
   redirect_stderr();
   err = search(trie, "NNNNNNNN", 8, hits, 0, 8);
   unredirect_stderr();
   reset_alloc();

   test_assert(err > 0);
   test_assert(hits[8]->nitems > hits[8]->nslots);

   reset_gstack(hits);

   // Do it again without failure.
   err = search(trie, "NNNNNNNN", 8, hits, 0, 8);
   test_assert(err == 0);

   destroy_trie(trie, DESTROY_NODES_YES, NULL);
   destroy_tower(hits);
   
}


void
test_mem_2
(void)
{

   srand48(123);
   char seq[21];
   seq[20] = '\0';

   // Construct 1000 tries with a 'malloc()' failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_alloc_failure_rate_to(0.01);

   redirect_stderr();
   for (int i = 0 ; i < 1000 ; i++) {
      trie_t *trie = new_trie(20);
      if (trie == NULL) {
         // Make sure errors are reported.
         test_assert(check_trie_error_and_reset() > 0);
         continue;
      }
      for (int j = 0 ; j < 100 ; j++) {
         for (int k = 0 ; k < 20 ; k++) {
            seq[k] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string(trie, seq);
         if (data == NULL) {
            // Make sure errors are reported.
            test_assert(check_trie_error_and_reset() > 0);
         }
         else {
            test_assert(*data == NULL);
            *data = data;
         }
      }
      destroy_trie(trie, DESTROY_NODES_YES, NULL);
   }

   unredirect_stderr();
   reset_alloc();
   
}


void
test_mem_3
(void)
{

   srand48(123);

   node_t *nodes = malloc(2000 * sizeof(node_t));
   if (nodes == NULL) {
      fprintf(stderr, "unittest error: %s:%d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }

   char seq[21];
   seq[20] = '\0';

   // Construct 1000 tries with a 'malloc()' failure rate of 1%.
   // If a segmentation fault happens the test will fail.
   set_alloc_failure_rate_to(0.01);

   redirect_stderr();
   for (int i = 0 ; i < 1000 ; i++) {
      trie_t *trie = new_trie(20);
      if (trie == NULL) {
         // Make sure errors are reported.
         test_assert(check_trie_error_and_reset() > 0);
         continue;
      }
      node_t *pos = nodes;
      for (int j = 0 ; j < 100 ; j++) {
         for (int k = 0 ; k < 20 ; k++) {
            seq[k] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string_wo_malloc(trie, seq, &pos);
         test_assert(*data == NULL);
         *data = data;
      }
      destroy_trie(trie, DESTROY_NODES_NO, NULL);
   }

   unredirect_stderr();
   reset_alloc();
   free(nodes);
   
}


void
test_mem_4
(void)
{

   int err;

   trie_t * trie = setup();
   gstack_t ** hits = new_tower(4);
   test_assert_critical(hits != NULL);

   // Test 'search()' 1000 times in a perfectly built trie
   // with a 'malloc()' failure rate of 1%.
   set_alloc_failure_rate_to(0.01);

   redirect_stderr();
   for (int i = 0 ; i < 1000 ; i++) {
      // The following search should not make any call to
      // 'malloc()' and so should have an error code of 0
      // on every call.
      reset_gstack(hits);
      err = search(trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 20);
      test_assert(err ==  0);
      test_assert(check_trie_error_and_reset() == 0);
      test_assert(hits[0]->nitems == 1);
      test_assert(hits[1]->nitems == 4);
      test_assert(hits[2]->nitems == 1);
      test_assert(hits[3]->nitems == 0);
   }

   unredirect_stderr();
   reset_alloc();
   destroy_tower(hits);
   teardown(trie);

}


void
test_mem_5
(void)
{

   srand48(123);

   // The variable 'trie' and 'hstack' are initialized
   // without 'malloc()' failure.
   trie_t *trie = new_trie(20);
   test_assert_critical(trie != NULL);

   gstack_t **hits = new_tower(4);
   test_assert_critical(hits != NULL);

   char seq[21];
   seq[20] = '\0';

   // Build an imperfect trie.
   for (int i = 0 ; i < 2048 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      void **data = insert_string(trie, seq);
      if (data == NULL) {
         test_assert(check_trie_error_and_reset() >  0);
      }
      else {
         *data = data;
      }
   }

   // Test 'search()' in the imperfectly built trie.
   for (int i = 0 ; i < 1000 ; i++) {
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      reset_gstack(hits);
      search(trie, seq, 3, hits, 0, 20);
   }

   unredirect_stderr();
   reset_alloc();
   destroy_tower(hits);
   destroy_trie(trie, DESTROY_NODES_YES, NULL);
   
}


void
test_mem_6
(void)
{

   trie_t *trie;
   node_t *trienode;
   gstack_t *gstack;
   gstack_t **tower;

   // Test constructors with a 'malloc()' failure rate of 1%.
   set_alloc_failure_rate_to(0.01);
   redirect_stderr();

   // Test 'new_trie()'.
   for (int i = 0 ; i < 1000 ; i++) {
      trie = new_trie(20);
      if (trie == NULL) {
         test_assert(check_trie_error_and_reset() >  0);
      }
      else {
         destroy_trie(trie, DESTROY_NODES_YES, NULL);
         trie = NULL;
      }
   }

   // Test 'new_trienode()'.
   for (int i = 0 ; i < 1000 ; i++) {
      trienode = new_trienode();
      if (trienode == NULL) {
         test_assert(check_trie_error_and_reset() >  0);
      }
      else {
         free(trienode);
         trienode = NULL;
      }
   }

   // Test 'new_gstack()'.
   for (int i = 0 ; i < 1000 ; i++) {
      gstack = new_gstack();
      if (gstack == NULL) {
         test_assert(check_trie_error_and_reset() >  0);
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
         test_assert(check_trie_error_and_reset() >  0);
      }
      else {
         destroy_tower(tower);
         tower = NULL;
      }
   }

   unredirect_stderr();
   reset_alloc();

}

// Test cases for export.
const test_case_t test_cases_trie[] = {
      {"trie/base/1", test_base_1},
      {"trie/base/2", test_base_2},
      {"trie/base/3", test_base_3},
      {"trie/base/4", test_base_4},
      {"trie/base/5", test_base_5},
      {"trie/base/6", test_base_6},
      {"trie/base/7", test_base_7},
      {"trie/base/8", test_base_8},
      {"errmsg",      test_errmsg},
      {"search",      test_search},
      {"mem/1",       test_mem_1},
      {"mem/2",       test_mem_2},
      {"mem/3",       test_mem_3},
      {"mem/4",       test_mem_4},
      {"mem/5",       test_mem_5},
      {"mem/6",       test_mem_6},
      {NULL, NULL},
};
