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

#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "view.h"
#include "trie.h"

#ifndef __STARCODE_LOADED_
#define __STARCODE_LOADED_

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

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

struct useq_t;
struct c_t;
struct match_t;

typedef struct useq_t useq_t;
typedef struct c_t ustack_t;
typedef struct match_t match_t;
typedef struct mtplan_t mtplan_t;
typedef struct mttrie_t mttrie_t;
typedef struct mtjob_t mtjob_t;
typedef struct lookup_t lookup_t;

typedef struct sortargs_t sortargs_t;

/*
struct ball_t;
typedef struct ball_t ball_t;
struct ball_t {
   double size;
   double position[2];
   double force[2];
   gstack_t *children;
   int starid;
};
*/

struct useq_t {
  int              count;
  char          *  seq;
  gstack_t      ** matches;
  struct useq_t *  canonical;
  // experimental //
  ball_t        *  ball;
};

struct lookup_t {
   int    offset;
   int    kmers;
   int  * klen;
   char * lut[];
};

struct sortargs_t {
   void ** buf0;
   void ** buf1;
   int     (*compar)(const void*, const void*);
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
   int		      clusters;
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

int starcode(FILE*, FILE*, const int, const int, const int, const int);
int tquery(FILE*, FILE*, FILE*, const int, const int, const int, const int);
void * tquery_thread(void*);
int bisection(int,int,char*,useq_t**,int,int);
match_t * new_match(useq_t *,int);

lookup_t *new_lookup(int, int, int, int);
int lut_search(lookup_t *, useq_t *);
void lut_insert(lookup_t *, useq_t *);
int seq2id(char *, int);
useq_t *new_useq(int, char*);
gstack_t *read_file(FILE*);
void destroy_useq(useq_t*);
void addmatch(useq_t*, useq_t*, int, int);
void transfer_counts_and_update_canonicals(useq_t*);
void unpad_useq (gstack_t*);
int pad_useq(gstack_t*, int*);
int canonical_order(const void*, const void*);
int count_order(const void *a, const void *b);
void *do_query(void*);
void run_plan(mtplan_t*, int, int);
mtplan_t *plan_mt(int, int, int, int, gstack_t*, const int);
void message_passing_clustering(gstack_t*, int);
void sphere_clustering(gstack_t*, int);
void * _mergesort(void *);
int seqsort(void **, int, int (*)(const void*, const void*), int);
long count_trie_nodes(useq_t **, int, int);
int AtoZ(const void *, const void *);

void view(gstack_t *);

#endif
