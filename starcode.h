#define _GNU_SOURCE
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "trie.h"

#ifndef __STARCODE_LOADED_
#define __STARCODE_LOADED_

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

int starcode(FILE*, FILE*, const int, const int, const int);
int tquery(FILE*, FILE*, FILE*, const int, const int);
#endif
