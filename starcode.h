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

#define STRATEGY_PREFIX 0
#define STRATEGY_EQUAL  1

#define min(a,b) (b < a ? b : a)
#define max(a,b) (a > b ? a : b)

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

struct u_t;
struct c_t;
struct m_t;

typedef struct u_t useq_t;
typedef struct c_t ustack_t;
typedef struct m_t match_t;
typedef struct mtplan_t mtplan_t;
typedef struct mttrie_t mttrie_t;
typedef struct mtjob_t mtjob_t;

typedef struct sortargs_t sortargs_t;

struct u_t {
          int    count;
          char * seq;
   struct c_t  * matches;
};

struct c_t {
          int    lim;
          int    pos;
          int    propidx;
   struct m_t  * u[];
};

struct m_t {
   struct u_t  * seq;
          int    dist;
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
   char              threadcount;
   int               numtries;
   struct mttrie_t * tries;
   pthread_mutex_t * mutex;
   pthread_cond_t  * monitor;
};

struct mttrie_t {
   char              flag;
   int               currentjob;
   int               numjobs;
   struct mtjob_t  * jobs;
};

struct mtjob_t {
   int                start;
   int                end;
   int                tau;
   int                build;
   useq_t          ** all_useq;
   node_t           * trie;
   pthread_mutex_t  * mutex;
   pthread_cond_t   * monitor;
   char             * trieflag;
   char             * threadcount;
};

int starcode(FILE*, FILE*, const int, const int, const int, const int, const int, const int);
int tquery(FILE*, FILE*, FILE*, const int, const int);
void * starcode_thread(void*);
mtplan_t * prepare_mtplan(int, int, int, int, useq_t**, int);
int bisection(int,int,char*,useq_t**,int,int);
void * _mergesort(void *);
int mergesort(void **, int, int (*)(const void*, const void*), int);
match_t * new_match(useq_t *,int);

#endif
