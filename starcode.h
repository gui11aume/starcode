#define _GNU_SOURCE
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "trie.h"

#ifndef __STARCODE_LOADED_
#define __STARCODE_LOADED_

#define NUMBASES 6
#define CONTEXT_STACK_SIZE 300
#define CONTEXT_STACK_OFFSET 100

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

struct u_t;
struct c_t;

typedef struct u_t useq_t;
typedef struct c_t ustack_t;

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
   int                  numconts;
   struct mtcontext_t * context;
}

struct mtcontext_t {
   int               numjobs;
   struct mtjob_t  * jobs;
}

struct mtjob_t {
   int                start;
   int                end;
   int                tau;
   useq_t          ** all_useq; 
   node_t           * trie;
   pthtread_t         thread;
};

int starcode(FILE*, FILE*, const int, const int, const int);
int tquery(FILE*, FILE*, FILE*, const int, const int);
void * starcode_thread(void*);
mtplan_t * prepare_mtplan(useq_t**,int);
#endif
