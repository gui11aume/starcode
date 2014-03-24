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

#define BISECTION_START  1
#define BISECTION_END    -1
#define TRIE_FREE 0
#define TRIE_BUSY 1
#define TRIE_DONE 2

#define min(a,b) (b < a ? b : a)

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

struct u_t;
struct c_t;

typedef struct u_t useq_t;
typedef struct c_t ustack_t;

typedef struct mtplan_t mtplan_t;
typedef struct mttrie_t mttrie_t;
typedef struct mtjob_t mtjob_t;

struct u_t {
          int    count;
          char * seq;
   struct c_t  * children;
};

struct c_t {
          int    lim;
          int    pos;
   struct u_t  * u[];
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

int starcode(FILE*, FILE*, const int, const int, const int);
int tquery(FILE*, FILE*, FILE*, const int, const int);
void * starcode_thread(void*);
mtplan_t * prepare_mtplan(int, int, int, int, useq_t**);
int bisection(int,int,char*,useq_t**,int,int);
#endif
