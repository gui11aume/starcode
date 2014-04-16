#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_

#define EOS -1             // End Of String -- for 'dash()'.
#define MAXBRCDLEN 127     // Maximum barcode length.
#define M 128              // MAXBRCDLEN + 1 for short.
#define STACK_INIT_SIZE 32 // Initial node size for narray and ustack


static const char BASES[8] = "ACGTN";

struct tnode_t;
struct tstack_t;
struct info_t;
struct hstack_t;

typedef struct tnode_t node_t;
typedef struct tstack_t narray_t;
typedef struct info_t info_t;
typedef struct hstack_t hstack_t;


int        check_trie_error_and_reset(void);
int        count_nodes(node_t*);
int        search(node_t*, const char*, int, hstack_t**, int, int);
void       destroy_trie(node_t*, void(*)(void *));
node_t   * new_trie(unsigned char, unsigned char);
node_t   * insert_string(node_t*, const char*);
narray_t * new_narray(void);
hstack_t * new_hstack(void);


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

struct arg_t {
   hstack_t ** hits;
   narray_t ** milestones;
   char        tau;
   char        maxtau;
   int       * query;
   int         trail;
   int         height;
   int         err;
};

struct tnode_t
{
            void     * data;           // Data (for tail nodes only).
   struct   tnode_t  * child[6];       // Array of 6 children pointers.
            uint32_t   path;           // Encoded path end to the node.
            char     * cache;          // Dynamic programming space.
};



struct tstack_t
{
            int        err;            // Trace memory errors.
            int        lim;            // Stack size.
            int        pos;            // Number of items.
   struct   tnode_t ** nodes;        // Nodes (items).
};

struct hstack_t
{
            int        err;
            int        lim;
            int        pos;
            int      * dist;
   struct   tnode_t ** nodes;
};


struct info_t
{
   unsigned char       maxtau;         // Max distance the trie can take.
            int        height;         // Critical depth with all hits.
   struct   tstack_t * milestones[M];  // Milestones for trail search.
};

// Search.
void _search(node_t*, int, struct arg_t);
void dash(node_t*, const int*, struct arg_t);
// Trie creation and destruction.
node_t *insert(node_t*, int, unsigned char);
node_t *new_trienode(unsigned char);
void init_milestones(node_t*);
void destroy_nodes_downstream_of(node_t*, void(*)(void*));
// Utility.
void push(node_t*, narray_t**);
void pushhit(node_t*, hstack_t**, int);
int check_trie_error_and_reset(void);

#endif
