/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
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

#ifndef _STARCODE_TRIE_PRIVATE_HEADER
#define _STARCODE_TRIE_PRIVATE_HEADER

#define PAD 5              // Position of padding nodes.
#define EOS -1             // End Of String, for 'dash()'.
// Translation tables between letters and numbers.
static const char untranslate[7] = "NACGT N";
// Translation table to insert nodes in the trie.
//          ' ': PAD (5)
//     'a', 'A': 1
//     'c', 'C': 2
//     'g', 'G': 3
//     't', 'T': 4
static const int translate[256] = { 
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 PAD,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,1,0,2,0,0,0,3,0,0,0,0,0,0,0,0,
   0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,
   0,1,0,2,0,0,0,3,0,0,0,0,0,0,0,0,
   0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
// Translation table to query a sequence in the trie.
// In the table below, non DNA letters are set to a numerical
// value of 6, which will always cause of mismatch with
// sequences translated from the table above. 'PAD' and '-' are
// the only non DNA symbols that match themselves.
//          ' ': PAD (5)
//          '-': 0
//     'a', 'A': 1
//     'c', 'C': 2
//     'g', 'G': 3
//     't', 'T': 4
static const int altranslate[256] = { 
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
 PAD,6,6,6,6,6,6,6,6,6,6,6,6,0,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,1,6,2,6,6,6,3,6,6,6,6,6,6,6,6,
   6,6,6,6,4,6,6,6,6,6,6,6,6,6,6,6,
   6,1,6,2,6,6,6,3,6,6,6,6,6,6,6,6,
   6,6,6,6,4,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
};

struct arg_t {
   gstack_t ** hits;
   gstack_t ** pebbles;
   char        tau;
   char        maxtau;
   int       * query;
   int         seed_depth;
   int         height;
   int         err;
};

void     dash (node_t*, const int*, struct arg_t);
void     destroy_from (node_t*, void(*)(void*), int, int, int);
int      get_height (trie_t*);
void     init_pebbles (node_t*);
node_t * insert (node_t *, int);
node_t * insert_wo_malloc (node_t *, int, node_t *);
node_t * new_trienode (void);
void     poucet (node_t*, int, struct arg_t);
int      recursive_count_nodes (node_t * node, int, int);

#endif
