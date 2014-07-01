#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "faultymalloc.h"
#include "trie.h"
#include "starcode.h"

// -- DECLARATION OF PRIVATE FUNCTIONS FROM trie.c -- //
int get_maxtau(trie_t*);
int get_height(trie_t*);
node_t *insert (node_t *, int, unsigned char);
void init_pebbles(node_t*);
node_t *new_trienode(char);
node_t *insert_wo_malloc(node_t *, int, unsigned char, void *);
// -- DECLARATION OF PRIVATE FUNCTIONS FROM starcode.c -- //
int AtoZ(const void *a, const void *b);
void addmatch(useq_t*, useq_t*, int, int);
void destroy_useq(useq_t*);
useq_t *new_useq(int, char *);
gstack_t *seq2useq(gstack_t*, int);
void transfer_counts_and_update_canonicals(useq_t*);
int canonical_order(const void*, const void*);
int seqsort (void **, int, int (*)(const void*, const void*), int);


typedef struct {
   trie_t *trie;
} fixture;


// Convenience functions.
int visit(node_t*, char*, int, int, int);
char *to_string (trie_t *trie, char* s) {
   visit(trie->root, s, 0, get_height(trie), 0);
   return s;
}
void reset(gstack_t **g) {
   for (int i = 0 ; g[i] != TOWER_TOP ; i++) g[i]->nitems = 0;
}

int
visit
(
   node_t *node,
   char *buff,
   int j,
   int maxdepth,
   int depth
)
{
   for (int i = 0 ; i < 6 ; i++) {
      if (node->child[i] != NULL) {
         buff[j++] = untranslate[i];
         if (depth < maxdepth-1) {
            node_t *child = (node_t *) node->child[i];
            j = visit(child, buff, j, maxdepth, depth+1);
         }
         else buff[j++] = '*';
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
   if (f->trie == NULL) {
      g_error("failed to initialize fixture\n");
   }
   for (int i = 0 ; i < 22 ; i++) {
      void **data = insert_string(f->trie, string[i]);
      // Set node data to non 'NULL'.
      if (data != NULL) {
         // Set the 'data' pointer to self.
         *data = data;
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
   destroy_trie(f->trie, DESTROY_NODES_YES, NULL);
}



// --  TEST FUNCTIONS -- //

void
test_base_1
(void)
// Test node creation and destruction.
{
   const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
   for (char maxtau = 0 ; maxtau < 9 ; maxtau++) {
      node_t *node = new_trienode(maxtau);
      // Check that creation succeeded.
      g_assert(node != NULL);
      // Check initialization.
      g_assert(node->cache != NULL);
      g_assert_cmpint(node->path, ==, 0);
      for (int i = 0 ; i < 6 ; i++) {
         g_assert(node->child[i] == NULL);
      }
      for (int i = 0 ; i < 2*maxtau + 1 ; i++) {
         g_assert(node->cache[i] == cache[i+(8-maxtau)]);
      }
      // Check no memory corruption.
      free(node);
   }
   return;
}


void
test_base_2
(void)
// Test 'insert()'.
{
   const char maxtau = 3;
   node_t *root = new_trienode(maxtau);
   g_assert(root != NULL);

   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert(root, i, maxtau);
      g_assert(node != NULL);
      for (int j = 0 ; j < 6 ; j++) {
         g_assert(node->child[j] == NULL);
      }
      const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
      for (int i = 0 ; i < 2*maxtau + 1 ; i++) {
         g_assert(node->cache[i] == cache[i+(8-maxtau)]);
      }
      g_assert_cmpint(node->path, ==, i);
      g_assert(root->child[i] == node);
   }

   // Destroy manually.
   for (int i = 0 ; i < 6 ; i++) free(root->child[i]);
   free(root);

   return;
}


void
test_base_3
(void)
// Test 'insert_wo_malloc()'.
{
   const char maxtau = 3;
   node_t *root = new_trienode(maxtau);
   g_assert(root != NULL);

   size_t cachesize = (2*maxtau + 1) * sizeof(char);
   node_t *nodes = malloc(6 * (sizeof(node_t) + cachesize));
   g_assert(nodes != NULL);

   for (int i = 0 ; i < 6 ; i++) {
      node_t *node = insert_wo_malloc(root, i, maxtau, nodes+i);
      g_assert(node != NULL);
      for (int j = 0 ; j < 6 ; j++) {
         g_assert(node->child[j] == NULL);
      }
      const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
      for (int i = 0 ; i < 2*maxtau + 1 ; i++) {
         g_assert(node->cache[i] == cache[i+(8-maxtau)]);
      }
      g_assert_cmpint(node->path, ==, i);
      g_assert(root->child[i] == node);
   }

   free(root);
   free(nodes);

   return;
}


void
test_base_4
(void)
// Test trie creation and destruction.
{

   for (char maxtau = 0 ; maxtau < 9 ; maxtau++) {
   for (int height = 0 ; height < M ; height++) {
      trie_t *trie = new_trie(maxtau, height);
      g_assert(trie != NULL);
      g_assert(trie->root != NULL);
      g_assert(trie->info != NULL);

      g_assert_cmpint(get_maxtau(trie), ==, maxtau);
      g_assert_cmpint(get_height(trie), ==, height);

      // Make sure that 'info' is initialized properly.
      info_t *info = trie->info;
      g_assert(((node_t*) *info->pebbles[0]->items) == trie->root);
      for (int i = 1 ; i < M ; i++) {
         g_assert(info->pebbles[i]->items != NULL);
      }

      // Insert 20 random sequences.
      for (int i = 0 ; i < 20 ; i++) {
         char seq[M] = {0};
         for (int j = 0 ; j < height ; j++) {
            seq[j] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string(trie, seq);
         g_assert(data != NULL);
         *data = data;
      }
      destroy_trie(trie, DESTROY_NODES_YES, NULL);
      trie = NULL;
   }
   }

   for (char maxtau = 0 ; maxtau < 9 ; maxtau++) {
   for (int height = 0 ; height < M ; height++) {
      trie_t *trie = new_trie(maxtau, height);
      g_assert(trie != NULL);
      g_assert(trie->root != NULL);
      g_assert(trie->info != NULL);

      g_assert_cmpint(get_maxtau(trie), ==, maxtau);
      g_assert_cmpint(get_height(trie), ==, height);

      // Make sure that 'info' is initialized properly.
      info_t *info = trie->info;
      g_assert(((node_t*) *info->pebbles[0]->items) == trie->root);
      for (int i = 1 ; i < M ; i++) {
         g_assert(info->pebbles[i]->items != NULL);
      }

      // Insert 20 random sequences without malloc.
      size_t cachesize = (2*maxtau + 1) * sizeof(char);
      node_t *nodes = malloc(20*height * (sizeof(node_t) + cachesize));
      g_assert(nodes != NULL);
      node_t *pos = nodes;
      for (int i = 0 ; i < 20 ; i++) {
         char seq[M] = {0};
         for (int j = 0 ; j < height ; j++) {
            seq[j] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string_wo_malloc(trie, seq, &pos);
         g_assert(data != NULL);
         *data = data;
      }
      destroy_trie(trie, DESTROY_NODES_NO, NULL);
      free(nodes);
      trie = NULL;
   }
   }
   return;
}


void
test_base_5
(void)
// Test 'insert_string()'.
{
   trie_t *trie = new_trie(3, 20);

   void **data = insert_string(trie, "AAAAAAAAAAAAAAAAAAAA");
   g_assert(data != NULL);
   *data = data;
   g_assert_cmpint(21, ==, count_nodes(trie));

   destroy_trie(trie, DESTROY_NODES_YES, NULL);
}


void
test_base_6
(void)
// Test 'insert_string_wo_malloc()'.
{
   const char maxtau = 3;
   trie_t *trie = new_trie(maxtau, 20);
   g_assert(trie != NULL);

   //size_t cachesize = (2*maxtau + 1) * sizeof(char);
   node_t *nodes = malloc(19 * (node_t_size(maxtau)));
   node_t *pos = nodes;
   void **data = 
      insert_string_wo_malloc(trie, "AAAAAAAAAAAAAAAAAAAA", &pos);
   g_assert(data != NULL);
   *data = data;
   g_assert_cmpint(21, ==, count_nodes(trie));
   // Check that the pointer has been incremented by 19 positions.
   int nnodes = ((char *)pos - (char *)nodes) / node_t_size(maxtau);
   g_assert_cmpint(19, ==, nnodes);

   // Cache upon initialization.
   const char cache[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
   // Successive 'paths' members of a line of A in the trie.
   const int paths[20] = {0,1,17,273,4369,69905,1118481,17895697,
      286331153,286331153,286331153,286331153,286331153,286331153,
      286331153,286331153,286331153,286331153,286331153,286331153};

   // Check the integrity of the nodes.
   node_t *node = trie->root;
   for (int i = 0 ; i < 20 ; i++) {
      g_assert(node != NULL);
      g_assert(node->cache != NULL);
      g_assert_cmpint(node->path, ==, paths[i]);
      g_assert(node->child[0] == NULL);
      g_assert(node->child[1] != NULL);
      for (int j = 2 ; j < 6 ; j++) {
         g_assert(node->child[j] == NULL);
      }
      for (int j = 0 ; j < 2*maxtau + 1 ; j++) {
         g_assert(node->cache[j] == cache[j+(8-maxtau)]);
      }
      node = node->child[1];
   }

   destroy_trie(trie, DESTROY_NODES_NO, NULL);
   free(nodes);

   return;

}


void
test_base_7
(void)
// Test 'new_gstack()' and 'push()'.
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
test_base_8
(void)
// Test 'new_tower()'.
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
   gstack_t **hits = new_tower(4);

   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 4);
   g_assert_cmpint(hits[2]->nitems, ==, 1);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, hits, 18, 3);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 3);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAATA", 3, hits, 3, 15);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 2);
   g_assert_cmpint(hits[3]->nitems, ==, 3);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAGACTG", 3, hits, 15, 15);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "AAAGAAAAAAAAAAAAAAAA", 3, hits, 15, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 3);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 19);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 3);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "TAAAAAAAAAAAAAAAAAAG", 3, hits, 19, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 1);
   g_assert_cmpint(hits[2]->nitems, ==, 4);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAA", 3, hits, 0, 1);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 4);
   g_assert_cmpint(hits[2]->nitems, ==, 1);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "   ATGCTAGGGTACTCGAT", 3, hits, 0, 1);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, " AAAAAAAAAAAAAAAAAAT", 3, hits, 1, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 4);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, hits, 0, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, " TGCTAGGGTACTCGATAAC", 1, hits, 0, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 1);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "NAAAAAAAAAAAAAAAAAAN", 2, hits, 0, 1);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 5);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "NNAAAAAAAAAAAAAAAANN", 3, hits, 1, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "AAAAAAAAAANAAAAAAAAA", 0, hits, 0, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "NNNAGACTTTTCCAGGGTAT", 3, hits, 0, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "GGGAGACTTTTCCAGGGNNN", 3, hits, 0, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "   GGGAGACTTTTCCAGGG", 3, hits, 0, 3);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "   AGACTTTTCCAGGGTAT", 3, hits, 3, 3);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

   reset(hits);
   search(f->trie, "                   N", 1, hits, 3, 19);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 3);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "                    ", 1, hits, 19, 0);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   // Caution here: the hit is present but the initial
   // search conditions are wrong.
   reset(hits);
   search(f->trie, "ATGCTAGGGTACTCGATAAC", 0, hits, 20, 1);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   // Repeat first test cases.
   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 18);
   g_assert_cmpint(hits[0]->nitems, ==, 1);
   g_assert_cmpint(hits[1]->nitems, ==, 4);
   g_assert_cmpint(hits[2]->nitems, ==, 1);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   reset(hits);
   search(f->trie, "AAAAAAAAAAAAAAAAAATA", 3, hits, 18, 18);
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 2);
   g_assert_cmpint(hits[2]->nitems, ==, 3);
   g_assert_cmpint(hits[3]->nitems, ==, 1);

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
   trie_t *trie = new_trie(9, 20);
   unredirect_sderr();
   g_assert(trie == NULL);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: 'maxtau' cannot be greater than 8\n");

   // Check error messages in 'insert()'.
   char too_long_string[M+2];
   too_long_string[M+1] = '\0';
   for (int i = 0 ; i < M+1 ; i++) too_long_string[i] = 'A';
   redirect_stderr_to(ERROR_BUFFER);
   void **not_inserted = insert_string(f->trie, too_long_string);
   unredirect_sderr();
   sprintf(string, "error: cannot insert string longer than %d\n",
         get_height(f->trie));
   g_assert(not_inserted == NULL);
   g_assert_cmpstr(ERROR_BUFFER, ==, string);
 
  
   // Check error messages in 'search()'.
   redirect_stderr_to(ERROR_BUFFER);
   err = search(f->trie, "AAAAAAAAAAAAAAAAAAAAA", 3, hits, 0, 0);
   g_assert_cmpint(err, ==, 80);
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
   g_assert_cmpint(err, ==, 74);
   g_assert_cmpstr(ERROR_BUFFER, ==,
         "error: requested tau greater than 'maxtau'\n");
   g_assert_cmpint(hits[0]->nitems, ==, 0);
   g_assert_cmpint(hits[1]->nitems, ==, 0);
   g_assert_cmpint(hits[2]->nitems, ==, 0);
   g_assert_cmpint(hits[3]->nitems, ==, 0);

   // Force 'malloc()' to fail and check error messages of constructors.
   redirect_stderr_to(ERROR_BUFFER);
   set_alloc_failure_rate_to(1);
   trie_t *trie_fail = new_trie(3, 20);
   reset_alloc();
   unredirect_sderr();
   g_assert(trie_fail == NULL);
   g_assert_cmpint(check_trie_error_and_reset(), > , 0);
   g_assert_cmpstr(ERROR_BUFFER, ==,
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

   trie_t *trie = new_trie(8, 8);
   g_assert(trie != NULL);

   gstack_t **hits = new_tower(9);
   g_assert(hits != NULL);
 
   int err = 0;

   // Insert 2049 random sequences.
   for (int i = 0 ; i < 2049 ; i++) {
      for (int j = 0 ; j < 8 ; j++) {
         seq[j] = untranslate[(int)(5 * drand48())];
      }
      void **data = insert_string(trie, seq);
      g_assert(data != NULL);
      *data = data;
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

   destroy_trie(trie, DESTROY_NODES_YES, NULL);
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
      trie_t *trie = new_trie(3, 20);
      if (trie == NULL) {
         // Make sure errors are reported.
         g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         continue;
      }
      for (int j = 0 ; j < 50 ; j++) {
         for (int k = 0 ; k < 20 ; k++) {
            seq[k] = untranslate[(int)(5 * drand48())];
         }
         void **data = insert_string(trie, seq);
         if (data == NULL) {
            // Make sure errors are reported.
            g_assert_cmpint(check_trie_error_and_reset(), >, 0);
         }
         else {
            // This can fail, but should not cause problem
            // when the trie is destroyed.
            g_assert(*data == NULL);
            *data = malloc(sizeof(char));
         }
      }
      destroy_trie(trie, DESTROY_NODES_YES, free);
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
   trie_t *trie = new_trie(3, 20);
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
      void **data = insert_string(trie, seq);
      if (data == NULL) {
         g_assert_cmpint(check_trie_error_and_reset(), > , 0);
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
   trie_t *trie;
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
         destroy_trie(trie, DESTROY_NODES_YES, NULL);
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
test_starcode_1
// Basic tests of useq.
(void)
{
   useq_t *case_1 = new_useq(1, "some sequence");
   g_assert(case_1 != NULL);
   g_assert_cmpint(case_1->count, ==, 1);
   g_assert_cmpstr(case_1->seq, ==, "some sequence");
   g_assert(case_1->matches == NULL);
   g_assert(case_1->canonical == NULL);

   useq_t *case_2 = new_useq(-1, "");
   g_assert(case_2 != NULL);
   g_assert_cmpint(case_2->count, ==, -1);
   g_assert_cmpstr(case_2->seq, ==, "");
   g_assert(case_2->matches == NULL);
   g_assert(case_2->canonical == NULL);

   addmatch(case_2, case_1, 1, 2);

   g_assert(case_1->matches == NULL);
   g_assert(case_2->matches != NULL);

   destroy_useq(case_1);
   destroy_useq(case_2);

   return;
}


void
test_starcode_2
(void)
// Test 'seqsort()'.
{

   // Case 1 (no repeat).
   char *sequences_1[10] = {
      "IRrLv<'*3S?UU<JF4S<,", "tcKvz5JTm!h*X0mSTg",
      "tW:0K&Mvtax<PP/qY6er", "hcU+f!=`.Xs6[a,C7XpN",
      ":3ILp'w?)f]4(a;mf%A9", "RlEF',$6[}ouJQyWqqT#",
      "U Ct`3w8(#KAE+z;vh,",  "[S^jXvNS VP' cwg~_iq",
      ".*/@*Q/]]}32kNB#`qqv", "#`Hwp(&,z|bN~07CSID'",
   };
   const char *sorted_1[10] = {
      "tcKvz5JTm!h*X0mSTg",   "U Ct`3w8(#KAE+z;vh,",
      "#`Hwp(&,z|bN~07CSID'", ".*/@*Q/]]}32kNB#`qqv",
      ":3ILp'w?)f]4(a;mf%A9", "IRrLv<'*3S?UU<JF4S<,",
      "RlEF',$6[}ouJQyWqqT#", "[S^jXvNS VP' cwg~_iq",
      "hcU+f!=`.Xs6[a,C7XpN", "tW:0K&Mvtax<PP/qY6er", 
   };

   useq_t *to_sort_1[10];
   for (int i = 0 ; i < 10 ; i++) {
      to_sort_1[i] = new_useq(1, sequences_1[i]);
   }

   seqsort((void **) to_sort_1, 10, AtoZ, 1);
   for (int i = 0 ; i < 10 ; i++) {
      g_assert_cmpstr(to_sort_1[i]->seq, ==, sorted_1[i]);
      g_assert_cmpint(to_sort_1[i]->count, ==, 1);
      free(to_sort_1[i]);
   }

   // Case 2 (repeats).
   char *sequences_2[6] = {
      "repeated", "repeated", "repeated", "repeated", "repeated", "xyz"
   };
   char *sorted_2[6] = {
      "xyz", "repeated", NULL, NULL, NULL, NULL,
   };
   int counts[2] = {1,5};

   useq_t *to_sort_2[6];
   for (int i = 0 ; i < 6 ; i++) {
      to_sort_2[i] = new_useq(1, sequences_2[i]);
   }

   seqsort((void **) to_sort_2, 6, AtoZ, 1);
   for (int i = 0 ; i < 2 ; i++) {
      g_assert_cmpstr(to_sort_2[i]->seq, ==, sorted_2[i]);
      g_assert_cmpint(to_sort_2[i]->count, ==, counts[i]);
      free(to_sort_2[i]);
   }
   for (int i = 2 ; i < 6 ; i++) {
      g_assert(to_sort_2[i] == NULL);
   }

   return;
}


void
test_starcode_3
(void)
// Test 'transfer_counts_and_update_canonicals'.
{
   useq_t *u1 = new_useq(1, "B}d2)$ChPyDC=xZ D-C");
   useq_t *u2 = new_useq(2, "RCD67vQc80:~@FV`?o%D");

   // Add match to 'u1'.
   addmatch(u1, u2, 1, 1);

   g_assert_cmpint(u1->count, ==, 1);
   g_assert_cmpint(u2->count, ==, 2);

   // This should not transfer counts but update canonical.
   transfer_counts_and_update_canonicals(u2);

   g_assert_cmpint(u1->count, ==, 1);
   g_assert_cmpint(u2->count, ==, 2);
   g_assert(u1->canonical == NULL);
   g_assert(u2->canonical == u2);

   // This should transfer the counts from 'u1' to 'u2'.
   transfer_counts_and_update_canonicals(u1);

   g_assert_cmpint(u1->count, ==, 0);
   g_assert_cmpint(u2->count, ==, 3);
   g_assert(u1->canonical == u2);
   g_assert(u2->canonical == u2);

   destroy_useq(u1);
   destroy_useq(u2);

   useq_t *u3 = new_useq(1, "{Lu[T}FOCMs}L_zx");
   useq_t *u4 = new_useq(2, "|kAV|Ch|RZ]h~WjCoDpX");
   useq_t *u5 = new_useq(2, "}lzHolUky");

   // Add matches to 'u3'.
   addmatch(u3, u4, 1, 1);
   addmatch(u3, u5, 1, 1);

   g_assert_cmpint(u3->count, ==, 1);
   g_assert_cmpint(u4->count, ==, 2);
   g_assert_cmpint(u5->count, ==, 2);

   transfer_counts_and_update_canonicals(u3);

   g_assert_cmpint(u3->count, ==, 0);
   g_assert_cmpint(u3->count, ==, 0);
   g_assert(u3->canonical == NULL);
   g_assert(u4->canonical == u4);
   g_assert(u5->canonical == u5);

   destroy_useq(u3);
   destroy_useq(u4);
   destroy_useq(u5);

   return;
}


void
test_starcode_4
(void)
// Test 'canonical_order'.
{
   useq_t *u1 = new_useq(1, "ABCD");
   useq_t *u2 = new_useq(2, "EFGH");

   g_assert_cmpint(canonical_order(u1, u2), ==, -4);
   g_assert_cmpint(canonical_order(u2, u1), ==, 4);

   // Add match to 'u1', and update canonicals.
   addmatch(u1, u2, 1, 1);
   transfer_counts_and_update_canonicals(u1);

   g_assert_cmpint(canonical_order(u1, u2), ==, -4);
   g_assert_cmpint(canonical_order(u2, u1), ==, 4);

   useq_t *u3 = new_useq(1, "CDEF");
   useq_t *u4 = new_useq(2, "GHIJ");

   g_assert_cmpint(canonical_order(u1, u3), ==, -1);
   g_assert_cmpint(canonical_order(u3, u1), ==, 1);
   g_assert_cmpint(canonical_order(u1, u4), ==, -1);
   g_assert_cmpint(canonical_order(u4, u1), ==, 1);
   g_assert_cmpint(canonical_order(u2, u3), ==, -1);
   g_assert_cmpint(canonical_order(u3, u2), ==, 1);
   g_assert_cmpint(canonical_order(u2, u4), ==, -1);
   g_assert_cmpint(canonical_order(u4, u2), ==, 1);
   g_assert_cmpint(canonical_order(u3, u4), ==, -4);
   g_assert_cmpint(canonical_order(u4, u3), ==, 4);

   // Add match to 'u3', and update canonicals.
   addmatch(u3, u4, 1, 1);
   transfer_counts_and_update_canonicals(u3);

   g_assert_cmpint(canonical_order(u1, u3), ==, -2);
   g_assert_cmpint(canonical_order(u3, u1), ==, 2);
   g_assert_cmpint(canonical_order(u1, u4), ==, -2);
   g_assert_cmpint(canonical_order(u4, u1), ==, 2);
   g_assert_cmpint(canonical_order(u2, u3), ==, -2);
   g_assert_cmpint(canonical_order(u3, u2), ==, 2);
   g_assert_cmpint(canonical_order(u2, u4), ==, -2);
   g_assert_cmpint(canonical_order(u4, u2), ==, 2);
   g_assert_cmpint(canonical_order(u3, u4), ==, -4);
   g_assert_cmpint(canonical_order(u4, u3), ==, 4);

   useq_t *u5 = new_useq(1, "CDEF");
   useq_t *u6 = new_useq(3, "GHIJ");

   g_assert_cmpint(canonical_order(u1, u5), ==, -1);
   g_assert_cmpint(canonical_order(u5, u1), ==, 1);
   g_assert_cmpint(canonical_order(u1, u6), ==, -1);
   g_assert_cmpint(canonical_order(u6, u1), ==, 1);
   g_assert_cmpint(canonical_order(u2, u5), ==, -1);
   g_assert_cmpint(canonical_order(u5, u2), ==, 1);
   g_assert_cmpint(canonical_order(u2, u6), ==, -1);
   g_assert_cmpint(canonical_order(u6, u2), ==, 1);
   g_assert_cmpint(canonical_order(u3, u5), ==, -1);
   g_assert_cmpint(canonical_order(u5, u3), ==, 1);
   g_assert_cmpint(canonical_order(u3, u6), ==, -1);
   g_assert_cmpint(canonical_order(u6, u3), ==, 1);
   g_assert_cmpint(canonical_order(u4, u5), ==, -1);
   g_assert_cmpint(canonical_order(u5, u4), ==, 1);
   g_assert_cmpint(canonical_order(u4, u6), ==, -1);
   g_assert_cmpint(canonical_order(u6, u4), ==, 1);
   g_assert_cmpint(canonical_order(u5, u6), ==, -4);
   g_assert_cmpint(canonical_order(u6, u5), ==, 4);

   // Add match to 'u5', and update canonicals.
   addmatch(u5, u6, 1, 1);
   transfer_counts_and_update_canonicals(u5);

   g_assert_cmpint(canonical_order(u1, u5), ==, 1);
   g_assert_cmpint(canonical_order(u5, u1), ==, -1);
   g_assert_cmpint(canonical_order(u1, u6), ==, 1);
   g_assert_cmpint(canonical_order(u6, u1), ==, -1);
   g_assert_cmpint(canonical_order(u2, u5), ==, 1);
   g_assert_cmpint(canonical_order(u5, u2), ==, -1);
   g_assert_cmpint(canonical_order(u2, u6), ==, 1);
   g_assert_cmpint(canonical_order(u6, u2), ==, -1);
   g_assert_cmpint(canonical_order(u3, u5), ==, 1);
   g_assert_cmpint(canonical_order(u5, u3), ==, -1);
   g_assert_cmpint(canonical_order(u3, u6), ==, 1);
   g_assert_cmpint(canonical_order(u6, u3), ==, -1);
   g_assert_cmpint(canonical_order(u4, u5), ==, 1);
   g_assert_cmpint(canonical_order(u5, u4), ==, -1);
   g_assert_cmpint(canonical_order(u4, u6), ==, 1);
   g_assert_cmpint(canonical_order(u6, u4), ==, -1);
   g_assert_cmpint(canonical_order(u5, u6), ==, -4);
   g_assert_cmpint(canonical_order(u6, u5), ==, 4);

   destroy_useq(u1);
   destroy_useq(u2);
   destroy_useq(u3);
   destroy_useq(u4);
   destroy_useq(u5);
   destroy_useq(u6);

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
   g_test_add_func("/base/7", test_base_7);
   g_test_add_func("/base/8", test_base_8);
   g_test_add("/search", fixture, NULL, setup, test_search, teardown);
   g_test_add("/errmsg", fixture, NULL, setup, test_errmsg, teardown);
   g_test_add("/mem/1", fixture, NULL, setup, test_mem_1, teardown);
   g_test_add("/mem/2", fixture, NULL, setup, test_mem_2, teardown);
   g_test_add("/mem/3", fixture, NULL, setup, test_mem_3, teardown);
   g_test_add("/mem/4", fixture, NULL, setup, test_mem_4, teardown);
   g_test_add_func("/starcode/1", test_starcode_1);
   g_test_add_func("/starcode/2", test_starcode_2);
   g_test_add_func("/starcode/3", test_starcode_3);
   g_test_add_func("/starcode/4", test_starcode_4);
   if (g_test_perf()) {
      g_test_add_func("/starcode/run", test_run);
   }
   return g_test_run();
}
