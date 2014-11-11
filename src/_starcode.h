/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
**
** Last modified: July 8, 2014
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

#ifndef _STARCODE_PRIVATE_HEADER
#define _STARCODE_PRIVATE_HEADER

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"

#define alert() fprintf(stderr, "error `%s' in %s() (%s:%d)\n",\
                     strerror(errno), __func__, __FILE__, __LINE__)

#define MAX_K_FOR_LOOKUP 14

#define PARENT_TO_CHILD_FACTOR  5
#define PARENT_TO_CHILD_MINIMUM 10

#define BISECTION_START  1
#define BISECTION_END    -1

#define TRIE_FREE 0
#define TRIE_BUSY 1
#define TRIE_DONE 2

#define STRATEGY_EQUAL  1
#define STRATEGY_PREFIX  99

#define str(a) (char *)(a)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

static const int valid_DNA_char[256] = { 
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,
   0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
   0,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,
   0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

struct useq_t;
struct c_t;
struct match_t;

typedef enum {
   FASTA,
   FASTQ,
   RAW,
   PE_FASTQ,
   UNSET,
} format_t;

typedef struct useq_t useq_t;
typedef struct c_t ustack_t;
typedef struct match_t match_t;
typedef struct mtplan_t mtplan_t;
typedef struct mttrie_t mttrie_t;
typedef struct mtjob_t mtjob_t;
typedef struct lookup_t lookup_t;

typedef struct sortargs_t sortargs_t;


struct useq_t {
  int              count;
  char          *  seq;
  char 		*  info;
  gstack_t      ** matches;
  struct useq_t *  canonical;
};

struct lookup_t {
            int    slen;
            int    kmers;
            int  * klen;
   unsigned char * lut[];
};


struct sortargs_t {
   useq_t ** buf0;
   useq_t ** buf1;
   int     size;
   int     b;
   int     thread;
   int     repeats;
};

struct mtplan_t {
   char              active;
   int               ntries;
   int		     jobsdone;
   struct mttrie_t * tries;
   pthread_mutex_t * mutex;
   pthread_cond_t  * monitor;
};

struct mttrie_t {
   char              flag;
   int               currentjob;
   int               njobs;
   struct mtjob_t  * jobs;
};

struct mtjob_t {
   int                start;
   int                end;
   int                tau;
   int                build;
   int                queryid;
   int                trieid;
   gstack_t         * useqS;
   trie_t           * trie;
   node_t           * node_pos;
   lookup_t         * lut;
   pthread_mutex_t  * mutex;
   pthread_cond_t   * monitor;
   int	    	    * jobsdone;
   char             * trieflag;
   char             * active;
};

int        size_order (const void *a, const void *b);
int        addmatch (useq_t*, useq_t*, int, int);
int        bisection (int, int, char *, useq_t **, int, int);
int        canonical_order (const void*, const void*);
long int   count_trie_nodes (useq_t **, int, int);
int        count_order (const void *, const void *);
void       destroy_useq (useq_t *);
void       destroy_lookup (lookup_t *);
void     * do_query (void*);
void       krash (void) __attribute__ ((__noreturn__));
int        lut_insert (lookup_t *, useq_t *); 
int        lut_search (lookup_t *, useq_t *); 
void       message_passing_clustering (gstack_t*, int);
lookup_t * new_lookup (int, int, int);
useq_t   * new_useq (int, char *, char *);
int        pad_useq (gstack_t*, int*);
mtplan_t * plan_mt (int, int, int, int, gstack_t *);
void       run_plan (mtplan_t *, int, int);
gstack_t * read_file (FILE *, FILE *, int);
int        seq2id (char *, int);
gstack_t * seq2useq (gstack_t*, int);
int        seqsort (useq_t **, int, int);
void       sphere_clustering (gstack_t *, int);
void       transfer_counts_and_update_canonicals (useq_t*);
void       unpad_useq (gstack_t*);
void     * nukesort (void *); 

#endif
