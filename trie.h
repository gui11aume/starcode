#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#ifndef __STARCODE_TRIE_LOADED_
#define __STARCODE_TRIE_LOADED_

#define DESTROY_NODES_YES 1
#define DESTROY_NODES_NO 0

static const char BASES[8] = "ACGTN";

struct gstack_t;
struct info_t;
struct node_t;
struct trie_t;

typedef struct gstack_t gstack_t;
typedef struct info_t info_t;
typedef struct node_t node_t;
typedef struct trie_t trie_t;

// Global constants.
#define PAD 5		   // Position of padding nodes.
#define EOS -1             // End Of String, for 'dash()'.
#define MAXBRCDLEN 127     // Maximum barcode length.
#define M 128              // MAXBRCDLEN + 1, for short.
#define STACK_INIT_SIZE 16 // Initial space for 'nstack' and 'hstack'
gstack_t * const TOWER_TOP;


int         search(trie_t*, const char*, int, gstack_t**, int, int);
trie_t   *  new_trie(unsigned char, unsigned char);
void     ** insert_string(trie_t*, const char*);
void        destroy_trie(trie_t*, int, void(*)(void *));
int         check_trie_error_and_reset(void);
int         count_nodes(trie_t*);
gstack_t *  new_gstack(void);
gstack_t ** new_tower(int);
void 	    destroy_tower(gstack_t **);
void 	    push(void*, gstack_t**);
void     ** insert_string_wo_malloc(trie_t *, const char *, node_t **);



// Translation tables between letters and numbers.
static const char untranslate[7] = "NACGT N";
static const int translate[256] = {
   [' '] = PAD, // The padding child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};
// In the table below, non DNA letters are set to a numerical
// value of 6, which will always cause of mismatch with
// sequences translated from the table above.
static const int altranslate[256] = {
   [0 ... 255] = 6,
   [' '] = PAD, // The padding child is in position 5.
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


struct arg_t {
//   hstack_t ** hits;
   gstack_t ** hits;
   gstack_t ** pebbles;
   char        tau;
   char        maxtau;
   int       * query;
   int         seed_depth;
   int         height;
   int         err;
};


struct trie_t
{
   node_t * root;
   info_t * info;
};


struct node_t
{
   void     * child[6];       // Array of 6 children pointers.
   uint32_t   path;           // Encoded path end to the node.
   char       cache[];        // Dynamic programming space.
};

struct gstack_t
{
            int       nslots;         // Stack size.
            int       nitems;         // Number of items.
   	    void    * items[];        // Items as 'void' pointers.
};


struct info_t
{
   unsigned char        maxtau;         // Max distance the trie can take.
            int         height;         // Critical depth with all hits.
   struct   gstack_t ** pebbles;        // White pebbles for the search.
};

#endif
