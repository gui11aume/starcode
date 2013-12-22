#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_


#define EOS 127         // End of translated string.
#define M 256           // MAXBRCDLEN + 1 for short.
#define MAXBRCDLEN 255  // Maximum barcode length.

struct tnode_t;
struct ans_t;

typedef struct tnode_t node_t;
typedef struct ans_t nstack_t;

node_t  * new_trienode (void);
node_t  * insert_string (node_t *, const char *);
void      triestack(node_t *, int, void(*)(node_t *));
void      destroy_nodes_downstream_of (node_t*, void(*)(void *));
int       check_trie_error_and_reset(void);
void      printrie (FILE *, node_t *);
void      printDYNP (FILE *, int);


// Translation tables between letters and numbers.
static const char untranslate[6] = "NACGTN";
static const int translate[256] = {
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


// Basic trie node data structure.
struct tnode_t
{
          int        pos;       // Position in parent array.
          int        depth;     // Depth in the trie.
   struct tnode_t  * parent;    // Parent node in the trie.
   struct tnode_t  * seen_by;   // Last visiting node.
   struct ans_t    * ans;       // Active node set.
          void     * data;      // Data for tail nodes.
   struct tnode_t  * child[5];  // Array of 5 children pointers.
};


// Stack of nodes.
struct ans_t
{
          int        lim;       // Size limit.
          int        idx;       // Current index.
   struct tnode_t  * node[];    // Array of nodes.
};
#endif
