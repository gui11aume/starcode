#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _starcode_trie_loaded
#define _starcode_trie_loaded

// End of translated string.
#define EOS 127

#define MAXBRCDLEN 255
#define M 256

// Translation tables between letters and numbers.
static const char untranslate[5] = "NACGT";
static const int translate[256] = {
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};
static const int altranslate[256] = {
   [0 ... 255] = -1,
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};

struct _node
{
   // Trie nodes contain a single DNA letter, A, C, G, T or anything
   // else. 'counter' is a positive 'int' for leaf nodes and 0 otherwise.
   // Trie nodes have a pointer to their parent and 5 pointers to
   // their children. They have a maximum of 5 children because all
   // non DNA letters are considered identical.
   void *data;
   struct _node *child[5];
};
typedef struct _node trienode;


typedef struct
{
   trienode *node;
   char match[MAXBRCDLEN];
   int  dist;
}
hit_t;


typedef struct
{
   // Structure to keep track of hits.
   int error;
   int size;
   int n_hits;
   hit_t *hits;
}
hip_t;


hip_t     *new_hip (void);
void       clear_hip (hip_t*);
void       destroy_hip (hip_t*);
trienode  *new_trienode (void);
trienode  *find_path (trienode *, const int **);
trienode  *insert_string (trienode *, const char *);
hip_t     *search (trienode *, const char *, int, hip_t*);
void       destroy_nodes_downstream_of (trienode*, void(*)(void *));
void       dfs(trienode *);
int        hip_error (hip_t *);
void       printrie(FILE *, trienode *);
void       printDYNP(FILE *, int);
#endif
