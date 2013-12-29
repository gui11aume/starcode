#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_


#define EOS 127         // End of translated string.
#define M 128           // MAXBRCDLEN + 1 for short.
#define MAXBRCDLEN 127  // Maximum barcode length.

struct tnode_t;
struct tstack_t;

typedef struct tnode_t node_t;
typedef struct tstack_t narray_t;

int        check_trie_error_and_reset(void);
void       destroy_nodes_downstream_of(node_t*, void(*)(void *));
node_t   * new_trienode(void);
node_t   * insert_string(node_t*, const char*);
narray_t * new_narray(void);
narray_t * search(node_t*, const char*, int, narray_t*);


// Translation tables between letters and numbers.
static const char untranslate[6] = "NACGTN";
static const int translate[256] = {
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};
static const int altranslate[256] = {
   [0 ... 255] = 5,
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


// Basic trie node data structure.
struct tnode_t
{
          void     * data;      // Data for tail nodes.
   struct tnode_t  * child[5];  // Array of 5 children pointers.
          int        path[3];
          int        cache[9];
};

struct tstack_t
{
          int        lim;       // Stack size.
          int        idx;       // Number of items.
   struct tnode_t  * nodes[];   // Items.
};
#endif
