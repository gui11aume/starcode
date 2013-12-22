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
int starcode(FILE*, FILE*, const int, const int);
#endif
