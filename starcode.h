#define _GNU_SOURCE
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "trie.h"

#if !defined( __GNUC__) || defined(__APPLE__)
   ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

int starcode(FILE*, FILE*, int, int);
