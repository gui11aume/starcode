/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (eduardvalera@gmail.com)
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

#ifndef _STARCODE_HEADER
#define _STARCODE_HEADER

#define _GNU_SOURCE
#include <stdio.h>
#include "trie.h"

#define VERSION          "starcode-v1.2 20-04-2018"
#define STARCODE_MAX_TAU 8

#define ERRM "starcode error:"

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

char * outname (char *);

struct useq_t;
typedef struct useq_t useq_t;
// The field 'seqid' is either an id number for
// the unique sequence or a pointer to a struct
// containing information about the matches. This
// creates some confusion in the code at times.
// See function 'transfer_useq_ids()'.
struct useq_t {
  int              count;       // Number of sequences
  unsigned int     nids;        // Number of associated IDs
  char          *  seq;         // Sequence
  char          *  info;        // Multi-function text field
  gstack_t      ** matches;     // Matches stratified by distance
  struct useq_t *  canonical;   // Pointer to canonical sequence
  int           *  seqid;       // Unique ID / pointer (see above).
};

useq_t   * new_useq (int, char *, char *);

typedef struct propt_t propt_t;
struct propt_t {
   char  first[5];
   int   pe_fastq;
   int   showclusters;
   int   showids;
};

typedef enum {
   DEFAULT_OUTPUT,
   CLUSTER_OUTPUT,
   NRED_OUTPUT
} output_t;

typedef enum {
   MP_CLUSTER,
   SPHERES_CLUSTER,
   COMPONENTS_CLUSTER
} cluster_t;

gstack_t *
read_file
(
   FILE      * inputf1,
   FILE      * inputf2,
   const int   verbose
);

typedef enum {
  INPUT_OK,
  NR_CL_ID_INCOMPATIBILITY,
  INPUT_INPUT12_INCOMPATIBILITY,
  ONLY_INPUT2_INCOMPATIBILITY,
  ONLY_INPUT1_INCOMPATIBILITY,
  NR_OUTPUT_INCOMPATIBILITY,
  SP_CP_INCOMPATIBILITY,
} input_compatibility_t;

input_compatibility_t
check_input
(
 int nr_flag,
 int cl_flag,
 int id_flag,
 int sp_flag,
 int cp_flag,
 int * threads,
 int * cluster_ratio,
 char *input1,
 char *input2,
 char *input,
 char *output
);

typedef struct {
  FILE *inputf1;
  FILE *inputf2;
  FILE *outputf1;
  FILE *outputf2;
} starcode_io_t;

output_t set_output_type (int);

typedef enum {
  IO_OK,
  IO_FILERR
} starcode_io_check;

starcode_io_check
set_input_and_output
(
 starcode_io_t *io,
 char * input1,
 char * input2,
 char * input,
 char * output1,
 char * output2,
 char * output,
 int nr_flag
);

cluster_t set_cluster_alg (int, int);

void say_usage (void);

void
head_default
(
   useq_t  * u,
   propt_t   propt
);

void
members_mp_default
(
   useq_t  * u,
   propt_t   propt
);
  
void
members_sc_default
(
   useq_t * u,
   propt_t  propt
);

// print functions
void print_ids (useq_t *, propt_t);
void print_nr_raw (useq_t *, propt_t);
void print_nr_fasta (useq_t *, propt_t);
void print_nr_fastq (useq_t *, propt_t);
void print_nr_pe_fastq (useq_t *, propt_t);

void
print_starcode_output
(
   FILE *outputf1,
   FILE *outputf2,
   gstack_t *clusters,
   const int clusteralg,
   const int showclusters,
   const int showids,
   const int outputt,
   const int verbose
);

int starcode(
   gstack_t *uSQ,
   FILE *outputf1,
   FILE *outputf2,
         int tau,
   const int verbose,
         int thrmax,
   const int clusteralg,
         int parent_to_child,
   const int showclusters,
   const int showids,
   const int outputt
);

#endif
