#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_

#define EOS -1          // End Of String -- for 'dash()'.
#define MAXBRCDLEN 127  // Maximum barcode length.
#define M 128           // MAXBRCDLEN + 1 for short.

struct a_node_t;
struct info_t;
struct node_t;
struct narray_t;

typedef struct a_node_t a_node_t;
typedef struct info_t info_t;
typedef struct node_t node_t;
typedef struct narray_t narray_t;

int        check_trie_error_and_reset(void);
a_node_t * compress_trie(node_t*);
int        count_nodes(node_t*);
void       destroy_trie(node_t*, void(*)(void *));
node_t   * insert_string(node_t*, const char*);
node_t   * new_trie(unsigned char, unsigned char);
narray_t * new_narray(void);
void       push(node_t*, narray_t**);
int        search(node_t*, const char*, int, narray_t**, int, int);


// Translation tables between letters and numbers.
static const char untranslate[7] = "NACGT N";
static const int translate[256] = {
   [' '] = 5, // The magic child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};
// In the table below, non DNA letters are set to a numerical
// value of 6, which will always cause of mismatch with
// sequences translated from the table above.
static const int altranslate[256] = {
   [0 ... 255] = 6,
   [' '] = 5, // The magic child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


struct node_t
{
            void     * data;           // Data (for tail nodes only).
   struct   node_t   * child[6];       // Array of 6 children pointers.
            uint32_t   path;           // Encoded path end to the node.
            char       cache[];        // Dynamic programming space.
};


struct a_node_t
{
// The 'a_node_t' structure is more compact than 'node_t', and is
// made to fit in an array. The children are indicated by their
// position in the array as an index.
            void     * data;           // Data (for tail nodes only).
            uint32_t   child[6];       // Array of 6 children index.
            uint32_t   path;           // Encoded path end to the node.
};


struct narray_t
{
            int        err;            // Trace memory errors.
            int        lim;            // Stack size.
            int        pos;            // Number of items.
   struct   node_t   * nodes[];        // Nodes (items).
};


struct info_t
{
   unsigned char       maxtau;         // Max distance the trie can take.
            int        height;         // Critical depth with all hits.
   struct   narray_t * milestones[M];  // Milestones for trail search.
};
#endif
