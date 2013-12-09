#define _GNU_SOURCE
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "trie.h"

// Maximum number of hits (parents).
#define MAXPAR 64

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif


struct _rel {
   int count;
   char *barcode;
   struct _rel *parents[MAXPAR];
};
typedef struct _rel star_t;


int
cmpstar
(
   const void *a,
   const void *b
)
{
   int A = (*(star_t **)a)->count;
   int B = (*(star_t **)b)->count;
   return (A < B) - (A > B);
}


int starcode(FILE*, FILE*, int, int, const int);
