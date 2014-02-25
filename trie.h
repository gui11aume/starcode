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

struct info_t;
struct nstash_t;
struct nstack_t;
struct node_t;
struct trie_t;

typedef struct info_t info_t;
typedef struct nstack_t nstack_t;
typedef struct nstash_t nstash_t;
typedef struct node_t node_t;
typedef struct trie_t trie_t;
typedef char* (DP_map_t)(const node_t*, const void*);

// Search.
int search
(
         trie_t   *  trie,
   const char     *  query,
   const int         tau,
         nstack_t ** hits,
         nstash_t *  stash,
   const int         start,
   const int         trail,
         DP_map_t *  DP_map,
   const void     *  maparg
);

int        check_trie_error_and_reset(void);
int        count_nodes(node_t*);
void       destroy_trie(trie_t*, void(*)(void *));
void       destroy_nstash(nstash_t*);
node_t   * insert_string(trie_t*, const char*);
char     * map_to_external(const node_t*, const void*);
char     * new_external_cache(uint32_t);
trie_t   * new_trie(unsigned char, unsigned char);
nstash_t * new_nstash_for_trie(trie_t*);
nstack_t * new_nstack(void);


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
   struct   node_t   * child[6];       // Array of 6 children pointers.
            uint32_t   path;           // Encoded path end to the node.
            uint32_t   numid;          // The numeric ID of the node.
            void     * data;           // Data (for tail nodes only).
            char       cache[];        // Dynamic programming space.
};


struct nstack_t
{
            int        err;            // Trace memory errors.
            int        lim;            // Stack size.
            int        pos;            // Number of items.
   struct   node_t   * nodes[];        // Nodes (items).
};


struct nstash_t
{
            int        size;
            nstack_t * slots[];
};


struct trie_t
{
   unsigned char       maxtau;         // Max distance the trie can take.
            int        height;         // Critical depth with all hits.
            uint32_t   size;           // Number of nodes in the trie.
            node_t   * root;           // Root of the trie.
};


struct info_t
{
   unsigned char       maxtau;         // Max distance the trie can take.
            int        height;         // Critical depth with all hits.
   struct   nstack_t * milestones[M];  // Milestones for trail search.
};
#endif
