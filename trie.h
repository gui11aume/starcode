#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_

#define EOS -1             // End Of String, for 'dash()'.
#define MAXBRCDLEN 127     // Maximum barcode length.
#define M 128              // MAXBRCDLEN + 1, for short.
#define STACK_INIT_SIZE 32 // Initial space for 'nstack' and 'hstack'


static const char BASES[8] = "ACGTN";

struct node_t;
struct nstack_t;
struct info_t;
struct hstack_t;
struct hit_t;

typedef struct node_t node_t;
typedef struct nstack_t nstack_t;
typedef struct info_t info_t;
typedef struct hstack_t hstack_t;
typedef struct hit_t hit_t;


int        search(node_t*, const char*, int, hstack_t**, int, int);
node_t   * new_trie(unsigned char, unsigned char);
node_t   * insert_string(node_t*, const char*);
void       destroy_trie(node_t*, void(*)(void *));
nstack_t * new_nstack(void);
hstack_t * new_hstack(void);
int        check_trie_error_and_reset(void);
int        count_nodes(node_t*);


// Translation tables between letters and numbers.
static const char untranslate[7] = "NACGT N";
static const int translate[256] = {
   [' '] = 5, // The padding child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};
// In the table below, non DNA letters are set to a numerical
// value of 6, which will always cause of mismatch with
// sequences translated from the table above.
static const int altranslate[256] = {
   [0 ... 255] = 6,
   [' '] = 5, // The padding child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


struct arg_t {
   hstack_t ** hits;
   nstack_t ** milestones;
   char        tau;
   char        maxtau;
   int       * query;
   int         trail;
   int         height;
   int         err;
};


struct node_t
{
            void    * data;           // Data (for tail nodes only).
   struct   node_t  * child[6];       // Array of 6 children pointers.
            uint32_t  path;           // Encoded path end to the node.
            char      cache[];        // Dynamic programming space.
};


struct nstack_t
{
            int       err;            // Trace memory errors.
            int       lim;            // Stack size.
            int       pos;            // Number of items.
   struct   node_t  * nodes[];        // Nodes (items).
};


struct hit_t
{
            int       dist;
            node_t  * node;
};


struct hstack_t
{
            int       err;            // Trace memory errors.
            int       lim;            // Stack size.
            int       pos;            // Number of hits.
   struct   hit_t     hits[];         // Hits.
};


struct info_t
{
   unsigned char       maxtau;         // Max distance the trie can take.
            int        height;         // Critical depth with all hits.
   struct   nstack_t * milestones[M];  // Milestones for trail search.
};

#endif
