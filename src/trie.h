/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (eduardvalera@gmail.com)
**
** License: 
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
*/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#ifndef _STARCODE_TRIE_HEADER
#define _STARCODE_TRIE_HEADER

#define DESTROY_NODES_YES 1
#define DESTROY_NODES_NO 0

#define gstack_size(elm) (2*sizeof(int) + elm * sizeof(void *))

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
#define TAU 8               // Max Levenshtein distance.
#define M 1024              // MAXBRCDLEN + 1, for short.
#define MAXBRCDLEN 1023     // Maximum barcode length.
#define GSTACK_INIT_SIZE 16 // Initial slots of 'gstack'.

gstack_t * const TOWER_TOP;

int         check_trie_error_and_reset (void);
int         count_nodes (trie_t*);
void        destroy_tower (gstack_t **);
void        destroy_trie (trie_t*, int, void(*)(void *));
void     ** insert_string_wo_malloc (trie_t *, const char *, node_t **);
void     ** insert_string (trie_t*, const char*);
gstack_t *  new_gstack (void);
gstack_t ** new_tower (int);
trie_t   *  new_trie (unsigned int);
int         push (void*, gstack_t**);
int         search (trie_t*, const char*, int, gstack_t**, int, int);

struct trie_t
{
   node_t * root;
   info_t * info;
};


struct node_t
{
   void     * child[6];             // Array of 6 children pointers.
   uint32_t   path;                 // Encoded path end to the node.
   char       cache[2*TAU+1];       // Dynamic programming space.
};

struct gstack_t
{
   int       nslots;                // Stack size.
   int       nitems;                // Number of items.
   void    * items[];               // Items as 'void' pointers.
};

struct info_t
{
   unsigned int         height;     // Critical depth with all hits.
   struct   gstack_t ** pebbles;    // White pebbles for the search.
};

#endif
