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

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"
#include "starcode.h"

#define alert() fprintf(stderr, "error `%s' in %s() (%s:%d)\n",\
                     strerror(errno), __func__, __FILE__, __LINE__)

#define MAX_K_FOR_LOOKUP 14

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

static const char capitalize[128] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
   48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
   64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
   96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127
};

struct c_t;
struct match_t;

typedef enum {
   FASTA,
   FASTQ,
   RAW,
   PE_FASTQ,
   UNSET,
} format_t;

typedef struct c_t ustack_t;
typedef struct match_t match_t;
typedef struct mtplan_t mtplan_t;
typedef struct mttrie_t mttrie_t;
typedef struct mtjob_t mtjob_t;
typedef struct lookup_t lookup_t;

typedef struct sortargs_t sortargs_t;


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
int        cluster_count (const void *, const void *);
gstack_t * compute_clusters (gstack_t *);
void       connected_components (useq_t *, gstack_t **);
long int   count_trie_nodes (useq_t **, int, int);
int        count_order (const void *, const void *);
int        count_order_spheres (const void *, const void *);
void       destroy_useq (useq_t *);
void       destroy_lookup (lookup_t *);
void     * do_query (void*);
int        int_ascending (const void*, const void*);
void       krash (void) __attribute__ ((__noreturn__));
int        lut_insert (lookup_t *, useq_t *); 
int        lut_search (lookup_t *, useq_t *); 
void       message_passing_clustering (gstack_t*, int);
lookup_t * new_lookup (int, int, int);
int        pad_useq (gstack_t*, int*);
mtplan_t * plan_mt (int, int, int, int, gstack_t *);
void       run_plan (mtplan_t *, int, int);
gstack_t * read_rawseq (FILE *, gstack_t *);
gstack_t * read_fasta (FILE *, gstack_t *, output_t);
gstack_t * read_fastq (FILE *, gstack_t *, output_t);
gstack_t * read_PE_fastq (FILE *, FILE *, gstack_t *, output_t);
int        seq2id (char *, int);
gstack_t * seq2useq (gstack_t*, int);
int        seqsort (useq_t **, int, int);
void       sphere_clustering (gstack_t *, int);
void       transfer_counts_and_update_canonicals (useq_t*, int);
void       transfer_useq_ids (useq_t *, useq_t *);
void       unpad_useq (gstack_t*);
void     * nukesort (void *); 
void       warn_about_missing_sequences (void);


//    Global variables    //
static FILE     * OUTPUTF1      = NULL;           // output file 1
static FILE     * OUTPUTF2      = NULL;           // output file 2
static format_t   FORMAT        = UNSET;          // input format
static cluster_t  CLUSTERALG    = MP_CLUSTER;     // cluster algorithm
static int        CLUSTER_RATIO = 5;              // min parent/child ratio
                                                  // to link clusters
char *
outname
(
   char *path
)
{

   char * name = calloc(320,1);
   if (strlen(path) > 310) {
      fprintf(stderr, "input file name too long (%s)\n", path);
      abort();
   }

   // Find final dot, append "-starcode" just before.
   // If no final dot, just append starcode as suffix.
   char *c = strrchr(path, '.');
   if (c == NULL) {
      sprintf(name, "%s-starcode", path);
   }
   else {
      *c = '\0';
      sprintf(name, "%s-starcode.%s", path, c+1);
      *c = '.';
   }

   return (char *) name;

}

void say_usage(void) { 
  char *USAGE =
  "\n"
  "Usage:"
  "  starcode [options]\n"
  "\n"
  "  general options:\n"
  "    -d --dist: maximum Levenshtein distance (default auto)\n"
  "    -t --threads: number of concurrent threads (default 1)\n"
  "    -q --quiet: quiet output (default verbose)\n"
  "    -v --version: display version and exit\n"
  "\n"
  "  cluster options: (default algorithm: message passing)\n"
  "    -r --cluster-ratio: min size ratio for merging clusters in\n"
  "               message passing (default 5)\n"
  "    -s --sphere: use sphere clustering algorithm\n"
  "    -c --connected-comp: cluster connected components\n"
  "\n"
  "  input/output options (single file, default)\n"
  "    -i --input: input file (default stdin)\n"
  "    -o --output: output file (default stdout)\n"
  "\n"
  "  input options (paired-end fastq files)\n"
  "    -1 --input1: input file 1\n"
  "    -2 --input2: input file 2\n"
  "\n"
  "  output options (paired-end fastq files, --non-redundant only)\n"
  "       --output1: output file1 (default input1-starcode.fastq)\n"
  "       --output2: output file2 (default input2-starcode.fastq)\n"
  "\n"
  "  output format options\n"
  "       --non-redundant: remove redundant sequences from input file(s)\n"
  "       --print-clusters: outputs cluster compositions\n"
  "       --seq-id: print sequence id numbers (1-based)\n";
  fprintf(stderr, "%s\n", USAGE);
}


void
head_default
(
   useq_t  * u,
   propt_t   propt
)
{

   useq_t * cncal = u->canonical;
   char * seq = propt.pe_fastq ? cncal->info : cncal->seq;

   fprintf(OUTPUTF1, "%s%s\t%d",
         propt.first, seq, cncal->count);

   if (propt.showclusters) {
      char * seq = propt.pe_fastq ? u->info : u->seq;
      fprintf(OUTPUTF1, "\t%s", seq);
   }

}

void
members_mp_default
(
   useq_t  * u,
   propt_t   propt
)
{

   if (!propt.showclusters) return;
   char * seq = propt.pe_fastq ? u->info : u->seq;
   fprintf(OUTPUTF1, ",%s", seq);

}

void
members_sc_default
(
   useq_t * u,
   propt_t  propt
)
{

   // Nothing to print if clusters are not shown, or
   // if this sequence has no match.
   if (!propt.showclusters || u->matches == NULL) return;

   gstack_t *hits;
   for (int j = 0 ; (hits = u->matches[j]) != TOWER_TOP ; j++) {
      for (int k = 0 ; k < hits->nitems ; k++) {
         useq_t *match = (useq_t *) hits->items[k];
         if (match->canonical != u) continue;
         char *seq = propt.pe_fastq ? match->seq : u->seq;
         fprintf(OUTPUTF1, ",%s", seq);
      }
   }

}


void
print_ids
(
   useq_t  * u,
   propt_t   propt
)
{
   (void) propt;

   // If there are more than one ID then 'u->seqid' is
   // a pointer to the IDs.
   if (u->nids > 1) {
      fprintf(OUTPUTF1, "\t%u", u->seqid[0]);
      for (unsigned int k = 1; k < u->nids; k++)
         fprintf(OUTPUTF1, ",%u", u->seqid[k]);
   }
   // If there is only one ID, then 'u->seqid' is that ID.
   else {
      // Guillaume Filion: the double cast has to do
      // with the fact that the high bit is used to
      // specify how the variable is used (as a value
      // or a point to a struct).
      fprintf(OUTPUTF1, "\t%u", (unsigned int) (unsigned long) u->seqid);
   }

}


void
print_nr_raw
(
   useq_t  * u,
   propt_t   propt
)
{
   (void) propt;
   fprintf(OUTPUTF1, "%s\n", u->seq);
}


void
print_nr_fasta
(
   useq_t  * u,
   propt_t   propt
)
{
   (void) propt;
   fprintf(OUTPUTF1, "%s\n%s\n", u->info, u->seq);
}


void
print_nr_fastq
(
   useq_t  * u,
   propt_t   propt
)
{
   (void) propt;
   char header[M] = {0};
   char quality[M] = {0};
   sscanf(u->info, "%s\n%s", header, quality);
   fprintf(OUTPUTF1, "%s\n%s\n+\n%s\n",
           header, u->seq, quality);
}
   

void
print_nr_pe_fastq
(
   useq_t  * u,
   propt_t   propt
)
{
   (void) propt;
   char head1[M] = {0};
   char head2[M] = {0};
   char qual1[M] = {0};
   char qual2[M] = {0};
   char seq1[M] = {0};
   char seq2[M] = {0};

   // Split the sequences.
   char *c = strrchr(u->seq, '-');
   strncpy(seq1, u->seq, c-u->seq - STARCODE_MAX_TAU);
   strcpy(seq2, c+1);

   // Split the info field.
   {
      char *c = u->info;
      strcpy(head1, strsep(&c, "\n"));
      strcpy(qual1, strsep(&c, "\n"));
      strcpy(head2, strsep(&c, "\n"));
      strcpy(qual2, strsep(&c, "\n"));
   }

   // Print to separate files.
   fprintf(OUTPUTF1, "%s\n%s\n+\n%s\n",
           head1, seq1, qual1);
   fprintf(OUTPUTF2, "%s\n%s\n+\n%s\n",
           head2, seq2, qual2);
}

void
warn_about_missing_sequences
(
   void
)
{
   fprintf(stderr,
         "warning: sequences that could not be unambiguously\n"
         "assigned to a cluster were removed from output\n");
   return;
}

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
)
{

   OUTPUTF1 = outputf1;
   OUTPUTF2 = outputf2;
   CLUSTERALG = clusteralg;

   propt_t propt = {
      .first         = {0},
      .showclusters  = showclusters,
      .showids       = showids,
      .pe_fastq      = PE_FASTQ == FORMAT,
   };

   if (CLUSTERALG == MP_CLUSTER) {

      // User must be warned when sequences without
      // canonical are removed from the output.
      int user_warned_about_missing_sequences = 0;

      if (outputt == DEFAULT_OUTPUT) {
         useq_t *first = (useq_t *) clusters->items[0];
         useq_t *canonical = first->canonical;

         // If the first canonical is NULL, then they all are.
         if (first->canonical == NULL) return;

         head_default(first, propt);

         // Use newline separator.
         memcpy(propt.first, "\n", 3);

         // Run through the clustered items.
         for (int i = 1 ; i < clusters->nitems ; i++) {
            useq_t *u = (useq_t *) clusters->items[i];
            if (u->canonical == NULL) {
               // Sequences without canonical are not printed
               // at all (but their counts are transferred).
               // Issue a warning when this happens, even if
               // verbose mode is off.
               if (!user_warned_about_missing_sequences) {
                  user_warned_about_missing_sequences = 1;
                  warn_about_missing_sequences();
               }
               break;
            }
            if (u->canonical != canonical) {
               // Print cluster seqIDs of previous canonical.
               if (showids) print_ids(canonical, propt);
               // Update canonical and print.
               canonical = u->canonical;
               head_default(canonical, propt);
            }
            else {
               members_mp_default(u, propt);
            }
         }

         // Print last cluster seqIDs.
         if (showids) print_ids(canonical, propt);
         fprintf(OUTPUTF1, "\n");
      }

   //
   //  SPHERES ALGORITHM
   // 

   } else if (clusteralg == SPHERES_CLUSTER) {
      // Default output.
      if (outputt == DEFAULT_OUTPUT) {
         for (int i = 0 ; i < clusters->nitems ; i++) {
            useq_t *u = (useq_t *) clusters->items[i];
            if (u->canonical != u) break;

            fprintf(OUTPUTF1, "%s\t", u->seq);
            if (showclusters) {
               fprintf(OUTPUTF1, "%d\t%s", u->count, u->seq);
            }
            else {
               fprintf(OUTPUTF1, "%d", u->count);
            }
            if (showclusters && u->matches != NULL) {
               gstack_t *hits;
               for (int j = 0 ; (hits = u->matches[j]) != TOWER_TOP ; j++) {
                  for (int k = 0 ; k < hits->nitems ; k++) {
                     useq_t *match = (useq_t *) hits->items[k];
                     if (match->canonical != u) continue;
                     fprintf(outputf1, ",%s", match->seq);
                  }
               }
            }
            // Print cluster seqIDs.
            if (showids) {
               if (u->nids > 1) {
                  fprintf(OUTPUTF1, "\t%u", u->seqid[0]);
               } else
                  // Leaving the double cast (see comment above).
                  fprintf(OUTPUTF1, "\t%u",
                     (unsigned int)(unsigned long)u->seqid);
               for (unsigned int k = 1; k < u->nids; k++) {
                  fprintf(OUTPUTF1, ",%u", u->seqid[k]);
               }
            }
            fprintf(OUTPUTF1, "\n");
         }
      }

   /*
    *  CONNECTED COMPONENTS ALGORITHM
    */

   } else if (clusteralg == COMPONENTS_CLUSTER) {
      // Default output.
      if (outputt == DEFAULT_OUTPUT) {
         for (int i = 0; i < clusters->nitems; i++) {
            gstack_t * cluster = (gstack_t *) clusters->items[i];
            // Get canonical.
            useq_t * canonical = (useq_t *) cluster->items[0];
            // Print canonical and cluster count.
            fprintf(outputf1, "%s\t%d", canonical->seq, canonical->count);
            if (showclusters) {
               fprintf (outputf1, "\t%s", canonical->seq);
               for (int k = 1; k < cluster->nitems; k++) {
                  fprintf (outputf1, ",%s", ((useq_t *)cluster->items[k])->seq);
               }
            }
            fprintf(outputf1, "\n");
         }
      }
   }

   /*
    * ALTERNATIVE OUTPUT FORMAT: NON-REDUNDANT
    */

   if (outputt == NRED_OUTPUT) {

      if (verbose) fprintf(stderr, "non-redundant output\n");
      // If print non redundant sequences, just print the
      // canonicals with their info.

      void (* print_nr) (useq_t *, propt_t) = {0};
           if (FORMAT == RAW)      print_nr = print_nr_raw;
      else if (FORMAT == FASTA)    print_nr = print_nr_fasta;
      else if (FORMAT == FASTQ)    print_nr = print_nr_fastq;
      else if (FORMAT == PE_FASTQ) print_nr = print_nr_pe_fastq;

      for (int i = 0 ; i < clusters->nitems ; i++) {
         useq_t *u = (useq_t *) clusters->items[i];
         if (u->canonical == NULL) break;
         if (u->canonical != u) continue;
         print_nr(u, propt);
      }

   }

   // Do not free anything.
   OUTPUTF1 = NULL;
   OUTPUTF2 = NULL;
}

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
 int input_set,
 int input1_set,
 int input2_set,
 int output_set
)
{
   // Check options compatibility. //
   if (nr_flag && (cl_flag || id_flag)) {
      fprintf(stderr,
            "%s --non-redundant flag is incompatible with "
            "--print-clusters and --seq-id\n", ERRM);
      say_usage();
      return NR_CL_ID_INCOMPATIBILITY;
   }
   if (input_set && (input1_set || input2_set)) {
      fprintf(stderr,
            "%s --input and --input1/2 are incompatible\n", ERRM);
      say_usage();
      return INPUT_INPUT12_INCOMPATIBILITY;
   }
   if (!input1_set && input2_set) {
      fprintf(stderr, "%s --input2 set without --input1\n", ERRM);
      printf ("input1_set = %d, input2_set = %d\n", input1_set, input2_set);
      say_usage();
      return ONLY_INPUT2_INCOMPATIBILITY;
   }
   if (!input2_set && input1_set) {
      fprintf(stderr, "%s --input1 set without --input2\n", ERRM);
      say_usage();
      return ONLY_INPUT1_INCOMPATIBILITY;
   }
   if (nr_flag && output_set &&
         (input1_set || input2_set)) {
      fprintf(stderr, "%s cannot specify --output for paired-end "
            "fastq file with --non-redundant\n", ERRM);
      say_usage();
      return NR_OUTPUT_INCOMPATIBILITY;
   }
   if (sp_flag && cp_flag) {
      fprintf(stderr, "%s --sphere and --connected-comp are "
              "incompatible\n", ERRM);
      say_usage();
      return SP_CP_INCOMPATIBILITY;
   }

   // Set remaining default options.
   if (*threads < 0) *threads = 1;
   if (*cluster_ratio < 0) *cluster_ratio = 5;

   // all went well: return
   return INPUT_OK;
}


output_t
set_output_type
(
 int nr_flag
)
{
   // Set output type. //
   int output_type;
   if      (nr_flag) output_type = NRED_OUTPUT;
   else              output_type = DEFAULT_OUTPUT;
   return output_type;
}

cluster_t set_cluster_alg
(
 int cp_flag,
 int sp_flag
)
{
   int cluster_alg;
   if      (cp_flag) cluster_alg = COMPONENTS_CLUSTER;
   else if (sp_flag) cluster_alg = SPHERES_CLUSTER;
   else              cluster_alg = MP_CLUSTER;
   return cluster_alg;
}


starcode_io_check
set_input_and_output
(
 starcode_io_t *io,
 char *input,
 char *input1,
 char *input2,
 char *output,
 char *output1,
 char *output2,
 int input1_set,
 int input2_set,
 int input_set,
 int output1_set,
 int output2_set,
 int output_set,
 int nr_flag
)
{
   // Set input file(s). //
   io->inputf1 = NULL;
   io->inputf2 = NULL;

   // Set output file(s). //
   io->outputf1 = NULL;
   io->outputf2 = NULL;

   if (input_set) {
      io->inputf1 = fopen(input, "r");
      if (io->inputf1 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input);
         say_usage();
         return IO_FILERR;
      }
   }
   else if (input1_set) {
      io->inputf1 = fopen(input1, "r");
      if (io->inputf1 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input1);
         say_usage();
         return IO_FILERR;
      }
      io->inputf2 = fopen(input2, "r");
      if (io->inputf2 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input2);
         say_usage();
         return IO_FILERR;
      }
   }
   else {
      io->inputf1 = stdin;
   }

   if (output_set) {
      io->outputf1 = fopen(output, "w");
      if (io->outputf1 == NULL) {
         fprintf(stderr, "%s cannot write to file %s\n", ERRM, output);
         say_usage();
         return IO_FILERR;
      }
   }
   else if (nr_flag && input1_set && input2_set) {
      // Set default names as inputX-starcode.fastq
      if (!output1_set) {
         output1 = outname(input1);
         io->outputf1 = fopen(output1, "w");
         free(output1);
      } else {
         io->outputf1 = fopen(output1, "w");
      }

      if (io->outputf1 == NULL) {
         fprintf(stderr,
               "%s cannot write to file %s\n", ERRM, outname(input1));
         say_usage();
         return IO_FILERR;
      }

      if (!output2_set) {
         output2 = outname(input2);
         io->outputf2 = fopen(output2, "w");
         free(output2);
      } else {
         io->outputf2 = fopen(output2, "w");
      }

      if (io->outputf2 == NULL) {
         fprintf(stderr,
               "%s cannot write to file %s\n", ERRM, outname(input2));
         say_usage();
         return IO_FILERR;
      }
   }
   else {
      io->outputf1 = stdout;
   }
   return IO_OK;
}

gstack_t *
starcode
(
   gstack_t *uSQ,
         int tau,
   const int verbose,
         int thrmax,
   const int clusteralg,
         int parent_to_child,
   const int showids
)
{

   CLUSTERALG = clusteralg;
   CLUSTER_RATIO = parent_to_child;

   // Sort/reduce.
   if (verbose) fprintf(stderr, "sorting\n");
   uSQ->nitems = seqsort((useq_t **) uSQ->items, uSQ->nitems, thrmax);

   // Get number of tries.
   int ntries = 3 * thrmax + (thrmax % 2 == 0);
   if (uSQ->nitems < ntries) {
      ntries = 1;
      thrmax = 1;
   }
 
   // Pad sequences (and return the median size).
   // Compute 'tau' from it in "auto" mode.
   int med = -1;
   int height = pad_useq(uSQ, &med);
   if (tau < 0) {
      tau = med > 160 ? 8 : 2 + med/30;
      if (verbose) {
         fprintf(stderr, "setting dist to %d\n", tau);
      }
   }
   
   // Make multithreading plan.
   mtplan_t *mtplan = plan_mt(tau, height, med, ntries, uSQ);

   // Run the query.
   run_plan(mtplan, verbose, thrmax);
   if (verbose) fprintf(stderr, "progress: 100.00%%\n");

   // Remove padding characters.
   unpad_useq(uSQ);

   //
   //  MESSAGE PASSING ALGORITHM
   //

   if (CLUSTERALG == MP_CLUSTER) {

      if (verbose) fprintf(stderr, "message passing clustering\n");

      // Cluster the pairs.
      message_passing_clustering(uSQ, showids);
      // Sort in canonical order.
      qsort(uSQ->items, uSQ->nitems, sizeof(useq_t *), canonical_order);

      return uSQ;

   //
   //  SPHERES ALGORITHM
   // 

   } else if (CLUSTERALG == SPHERES_CLUSTER) {

      if (verbose) fprintf(stderr, "spheres clustering\n");
      // Cluster the pairs.
      sphere_clustering(uSQ, showids);
      // Sort in count order.
      qsort(uSQ->items, uSQ->nitems, sizeof(useq_t *), count_order);

      return uSQ;

   /*
    *  CONNECTED COMPONENTS ALGORITHM
    */

   } else if (CLUSTERALG == COMPONENTS_CLUSTER) {
      if (verbose) fprintf(stderr, "connected components clustering\n");
      // Cluster connected components.
      // Returns a stack containing stacks of clusters, where clusters->item[i]->item[0] is
      // the centroid of the i-th cluster. The output is sorted by cluster count, which is
      // stored in centroid->count.
      return compute_clusters(uSQ);
   }

   return NULL;
}

void
run_plan
(
   mtplan_t *mtplan,
   const int verbose,
   const int thrmax
)
{
   // Count total number of jobs.
   int njobs = mtplan->ntries * (mtplan->ntries+1) / 2;

   // Thread Scheduler
   int triedone = 0;
   int idx = -1;

   while (triedone < mtplan->ntries) { 
      // Cycle through the tries in turn.
      idx = (idx+1) % mtplan->ntries;
      mttrie_t *mttrie = mtplan->tries + idx;
      pthread_mutex_lock(mtplan->mutex);     

      // Check whether trie is idle and there are available threads.
      if (mttrie->flag == TRIE_FREE && mtplan->active < thrmax) {

         // No more jobs on this trie.
         if (mttrie->currentjob == mttrie->njobs) {
            mttrie->flag = TRIE_DONE;
            triedone++;
         }

         // Some more jobs to do.
         else {
            mttrie->flag = TRIE_BUSY;
            mtplan->active++;
            mtjob_t *job = mttrie->jobs + mttrie->currentjob++;
            pthread_t thread;
            // Start job and detach thread.
            if (pthread_create(&thread, NULL, do_query, job)) {
               alert();
               krash();
            }
            pthread_detach(thread);
            if (verbose) {
               fprintf(stderr, "progress: %.2f%% \r",
                     100*(float)(mtplan->jobsdone)/njobs);
            }
         }
      }

      // If max thread number is reached, wait for a thread.
      while (mtplan->active == thrmax) {
         pthread_cond_wait(mtplan->monitor, mtplan->mutex);
      }

      pthread_mutex_unlock(mtplan->mutex);

   }

   return;

}


void *
do_query
(
   void * args
)  
{
   // Unpack arguments.
   mtjob_t  * job    = (mtjob_t*) args;
   gstack_t * useqS  = job->useqS;
   trie_t   * trie   = job->trie;
   lookup_t * lut    = job->lut;
   const int  tau    = job->tau;
   node_t * node_pos = job->node_pos;

   // Create local hit stack.
   gstack_t **hits = new_tower(tau+1);
   if (hits == NULL) {
      alert();
      krash();
   }

   // Define a constant to help the compiler recognize
   // that only one of the two cases will ever be used
   // in the loop below.
   const int bidir_match = (CLUSTERALG == SPHERES_CLUSTER || CLUSTERALG == COMPONENTS_CLUSTER);
   useq_t * last_query = NULL;

   for (int i = job->start ; i <= job->end ; i++) {
      useq_t *query = (useq_t *) useqS->items[i];
      int do_search = lut_search(lut, query) == 1;

      // Insert the new sequence in the lut and trie, but let
      // the last pointer to NULL so that the query does not
      // find itself upon search.
      void **data = NULL;
      if (job->build) {
         if (lut_insert(lut, query)) {
            alert();
            krash();
         }
         data = insert_string_wo_malloc(trie, query->seq, &node_pos);
         if (data == NULL || *data != NULL) {
            alert();
            krash();
         }
      }

      if (do_search) {
         int trail = 0;
         if (i < job->end) {
            useq_t *next_query = (useq_t *) useqS->items[i+1];
            // The 'while' condition is guaranteed to be false
            // before the end of the 'char' arrays because all
            // the queries have the same length and are different.
            while (query->seq[trail] == next_query->seq[trail]) {
               trail++;
            }
         }

         // Compute start height.
         int start = 0;
         if (last_query != NULL) {
            while(query->seq[start] == last_query->seq[start]) start++;
         }

         // Clear hit stack. //
         for (int j = 0 ; hits[j] != TOWER_TOP ; j++) {
            hits[j]->nitems = 0;
         }

         // Search the trie. //
         int err = search(trie, query->seq, tau, hits, start, trail);
         if (err) {
            alert();
            krash();
         }

         for (int j = 0 ; hits[j] != TOWER_TOP ; j++) {
            if (hits[j]->nitems > hits[j]->nslots) {
               fprintf(stderr, "warning: incomplete search (%s)\n",
                       query->seq);
               break;
            }
         }

         // Link matching pairs for clustering.
         // Skip dist = 0, as this would be self.
         for (int dist = 1 ; dist < tau+1 ; dist++) {
         for (int j = 0 ; j < hits[dist]->nitems ; j++) {

            useq_t *match = (useq_t *) hits[dist]->items[j];
            if (bidir_match) {
               // Make a bidirectional match reference.
               // Add reference from query to matched node.
               pthread_mutex_lock(job->mutex + job->queryid);
               if (addmatch(query, match, dist, tau)) {
                  fprintf(stderr,
                        "Please contact guillaume.filion@gmail.com "
                        "for support with this issue.\n");
                  abort();
               }
               pthread_mutex_unlock(job->mutex + job->queryid);
               // Add reference from matched node to query.
               pthread_mutex_lock(job->mutex + job->trieid);
               if (addmatch(match, query, dist, tau)) {
                  fprintf(stderr,
                        "Please contact guillaume.filion@gmail.com "
                        "for support with this issue.\n");
                  abort();
               }
               pthread_mutex_unlock(job->mutex + job->trieid);
            }

            else {
               // The parent is the sequence with highest count.
               // For ties, parent is the query and child is the match.
               // Matches are stored in child only (i.e. children
               // know their parents).
               useq_t *parent = match->count > query->count ? match : query;
               useq_t *child  = match->count > query->count ? query : match;
               // If clustering is done by message passing, do not link
               // pair if counts are on the same order of magnitude.
               int mincount = child->count;
               int maxcount = parent->count;
               if (maxcount < CLUSTER_RATIO * mincount) continue;
               // The child is modified, use the child mutex.
               int mutexid = match->count > query->count ?
                             job->queryid : job->trieid;
               pthread_mutex_lock(job->mutex + mutexid);
               if (addmatch(child, parent, dist, tau)) {
                  fprintf(stderr,
                        "Please contact guillaume.filion@gmail.com "
                        "for support with this issue.\n");
                  abort();
               }
               pthread_mutex_unlock(job->mutex + mutexid);
            }
         }
         }

         last_query = query;

      }

      if (job->build) {
         // Finally set the pointer of the inserted tail node.
         *data = query;
      }
   }
   
   destroy_tower(hits);

   // Flag trie, update thread count and signal scheduler.
   // Use the general mutex. (job->mutex[0])
   pthread_mutex_lock(job->mutex);
   *(job->active) -= 1;
   *(job->jobsdone) += 1;
   *(job->trieflag) = TRIE_FREE;
   pthread_cond_signal(job->monitor);
   pthread_mutex_unlock(job->mutex);

   return NULL;

}


mtplan_t *
plan_mt
(
    int       tau,
    int       height,
    int       medianlen,
    int       ntries,
    gstack_t *useqS
)
// SYNOPSIS:                                                              
//   The scheduler makes the key assumption that the number of tries is   
//   an odd number, which allows to distribute the jobs among as in the   
//   example shown below. The rows indicate blocks of query strings and   
//   the columns are distinct tries. An circle (o) indicates a build job, 
//   a cross (x) indicates a query job, and a dot (.) indicates that the  
//   block is not queried in the given trie.                              
//                                                                        
//                            --- Tries ---                               
//                            1  2  3  4  5                               
//                         1  o  .  .  x  x                               
//                         2  x  o  .  .  x                               
//                         3  x  x  o  .  .                               
//                         4  .  x  x  o  .                               
//                         5  .  .  x  x  o                               
//                                                                        
//   This simple schedule ensures that each trie is built from one query  
//   block and that each block is queried against every other exactly one 
//   time (a query of block i in trie j is the same as a query of block j 
//   in trie i).                                                          
{
   // Initialize plan.
   mtplan_t *mtplan = malloc(sizeof(mtplan_t));
   if (mtplan == NULL) {
      alert();
      krash();
   }

   // Initialize mutex.
   pthread_mutex_t *mutex = malloc((ntries + 1) * sizeof(pthread_mutex_t));
   pthread_cond_t *monitor = malloc(sizeof(pthread_cond_t));
   if (mutex == NULL || monitor == NULL) {
      alert();
      krash();
   }
   for (int i = 0; i < ntries + 1; i++) pthread_mutex_init(mutex + i,NULL);
   pthread_cond_init(monitor,NULL);

   // Initialize 'mttries'.
   mttrie_t *mttries = malloc(ntries * sizeof(mttrie_t));
   if (mttries == NULL) {
      alert();
      krash();
   }

   // Boundaries of the query blocks.
   int Q = useqS->nitems / ntries;
   int R = useqS->nitems % ntries;
   int *bounds = malloc((ntries+1) * sizeof(int));
   for (int i = 0 ; i < ntries+1 ; i++) bounds[i] = Q*i + min(i, R);

   // Preallocated tries.
   // Count with maxlen-1
   long *nnodes = malloc(ntries * sizeof(long));
   for (int i = 0; i < ntries; i++) nnodes[i] =
      count_trie_nodes((useq_t **)useqS->items, bounds[i], bounds[i+1]);

   // Create jobs for the tries.
   for (int i = 0 ; i < ntries; i++) {
      // Remember that 'ntries' is odd.
      int njobs = (ntries+1)/2;
      trie_t *local_trie  = new_trie(height);
      node_t *local_nodes = (node_t *) malloc(nnodes[i] * sizeof(node_t));
      mtjob_t *jobs = malloc(njobs * sizeof(mtjob_t));
      if (local_trie == NULL || jobs == NULL) {
         alert();
         krash();
      }

      // Allocate lookup struct.
      // TODO: Try only one lut as well. (It will always return 1
      // in the query step though). #
      lookup_t * local_lut = new_lookup(medianlen, height, tau);
      if (local_lut == NULL) {
         alert();
         krash();
      }

      mttries[i].flag       = TRIE_FREE;
      mttries[i].currentjob = 0;
      mttries[i].njobs      = njobs;
      mttries[i].jobs       = jobs;

      for (int j = 0 ; j < njobs ; j++) {
         // Shift boundaries in a way that every trie is built
         // exactly once and that no redundant jobs are allocated.
         int idx = (i+j) % ntries;
         int only_if_first_job = j == 0;
         // Specifications of j-th job of the local trie.
         jobs[j].start    = bounds[idx];
         jobs[j].end      = bounds[idx+1]-1;
         jobs[j].tau      = tau;
         jobs[j].build    = only_if_first_job;
         jobs[j].useqS    = useqS;
         jobs[j].trie     = local_trie;
         jobs[j].node_pos = local_nodes;
         jobs[j].lut      = local_lut;
         jobs[j].mutex    = mutex;
         jobs[j].monitor  = monitor;
         jobs[j].jobsdone = &(mtplan->jobsdone);
         jobs[j].trieflag = &(mttries[i].flag);
         jobs[j].active   = &(mtplan->active);
         // Mutex ids. (mutex[0] is reserved for general mutex)
         jobs[j].queryid  = idx + 1;
         jobs[j].trieid   = i + 1;
      }
   }

   free(bounds);

   mtplan->active = 0;
   mtplan->ntries = ntries;
   mtplan->jobsdone = 0;
   mtplan->mutex = mutex;
   mtplan->monitor = monitor;
   mtplan->tries = mttries;

   return mtplan;

}

long
count_trie_nodes
(
 useq_t ** seqs,
 int     start,
 int     end
)
{
   int  seqlen = strlen(seqs[start]->seq) - 1;
   long count = seqlen;
   for (int i = start+1; i < end; i++) {
      char * a = seqs[i-1]->seq;
      char * b  = seqs[i]->seq;
      int prefix = 0;
      while (a[prefix] == b[prefix]) prefix++;
      count += seqlen - prefix;
   }
   return count;
}

void
connected_components
(
 useq_t * useq,
 gstack_t ** cluster
)
{
   // Flag claimed.
   useq->canonical = useq;
   // Add myself to cluster.
   push(useq, cluster);
   // Recursive call on edges.
   if (useq->matches == NULL) return;
   gstack_t * matches;
   for (int j = 0;  (matches = useq->matches[j]) != TOWER_TOP; j++) {
      for (int k = 0; k < matches->nitems; k++) {
         useq_t * match = (useq_t *) matches->items[k];
         if (match->canonical != NULL) continue;
         connected_components(match, cluster);
      }
   }
}

gstack_t *
compute_clusters
(
 gstack_t    * uSQ
)
{
   gstack_t * clusters = new_gstack();
   for (int i = 0; i < uSQ->nitems; i++) {
      useq_t * useq = (useq_t *) uSQ->items[i];

      // Check sequence flag.
      if (useq->canonical != NULL) continue;

      // Create new cluster.
      gstack_t * cluster = new_gstack();

      // Recursively gather connected components.
      connected_components(useq, &cluster);

      // Find centroid. (max: #counts THEN #edges).
      // Count useq edges.
      int edge_count = 0;
      if (useq->matches != NULL) {
         gstack_t * matches;
         for (int j = 0; (matches = useq->matches[j]) != TOWER_TOP; j++)
            edge_count += matches->nitems;
      }

      // Find centroid among cluster seqs.
      size_t cluster_count = useq->count;
      for (int k = 1; k < cluster->nitems; k++) {
         useq_t * s = (useq_t *) cluster->items[k];
         cluster_count += s->count;
         // Select centroid by count.
         if (s->count > useq->count) {
            // Count centroid edges.
            int cnt = 0;
            gstack_t * matches;
            for (int j = 0; (matches = s->matches[j]) != TOWER_TOP; j++)
               cnt += matches->nitems;
            // Store centroid at index 0.
            cluster->items[0] = s;
            cluster->items[k] = useq;
            useq = s;
            // Save centroid edge count.
            edge_count = cnt;
         }
         // If same count, select by edge count.
         else if (s->count == useq->count && s->matches != NULL) {
            int cnt = 0;
            gstack_t * matches;
            for (int j = 0; (matches = s->matches[j]) != TOWER_TOP; j++)
               cnt += matches->nitems;
            if (cnt > edge_count) {
               // Store centroid at index 0.
               cluster->items[0] = s;
               cluster->items[k] = useq;
               useq = s;
               // Save centroid edge count.
               edge_count = cnt;

            }
         }
      }
      useq->count = cluster_count;
      // Store cluster.
      push(cluster, &clusters);
   }

   // Sort clusters by size (counts).
   qsort(clusters->items, clusters->nitems, sizeof(gstack_t *), cluster_count);

   return clusters;
}


void
sphere_clustering
(
 gstack_t *useqS,
 int transfer_ids
)
{
   // Sort in count order.
   qsort(useqS->items, useqS->nitems, sizeof(useq_t *), count_order_spheres);

   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *useq = (useq_t *) useqS->items[i];
      if (useq->canonical != NULL) continue;
      useq->canonical = useq;
      if (useq->matches == NULL) continue;
      // Bidirectional edge references simplifie the algorithm.
      // Directly proceed to claim neighbor counts.
      gstack_t *matches;
      for (int j = 0 ; (matches = useq->matches[j]) != TOWER_TOP ; j++) {
         for (int k = 0 ; k < matches->nitems ; k++) {
            useq_t *match = (useq_t *) matches->items[k];
            // If a sequence has been already claimed, remove it from list.
            if (match->canonical != NULL) {
               matches->items[k--] = matches->items[--matches->nitems];
               continue;
            }
            // Otherwise, claim the sequence.
            match->canonical = useq;
            useq->count += match->count;
            match->count = 0;
            if (transfer_ids)
               transfer_useq_ids(useq, match);
         }
      }
   }

   return;

}


void
message_passing_clustering
(
 gstack_t *useqS,
 int transfer_ids
)
{
   // Transfer counts to parents recursively.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = (useq_t *) useqS->items[i];
      transfer_counts_and_update_canonicals(u, transfer_ids);
   }

   return;

}


int
seqsort
(
 useq_t ** data,
 int       numels,
 int       thrmax
)
// SYNOPSIS:                                                              
//   Recursive merge sort for 'useq_t' arrays, tailored for the
//   problem of sorting merging identical sequences. When two
//   identical sequences are detected during the sort, they are
//   merged into a single one with more counts, and one of them
//   is destroyed (freed). See 'nukesort()' for a description of
//   the sort order.
//
// PARAMETERS:                                                            
//   data:       an array of pointers to each element.                    
//   numels:     number of elements, i.e. size of 'data'.                 
//   thrmax: number of threads.                                       
//                                                                        
// RETURN:                                                                
//   Number of unique elements.                               
//                                                                        
// SIDE EFFECTS:                                                          
//   Pointers to repeated elements are set to NULL.
{
   // Copy to buffer.
   useq_t **buffer = malloc(numels * sizeof(useq_t *));
   memcpy(buffer, data, numels * sizeof(useq_t *));

   // Prepare args struct.
   sortargs_t args;
   args.buf0   = data;
   args.buf1   = buffer;
   args.size   = numels;
   // There are two alternating buffers for the merge step.
   // 'args.b' alternates on every call to 'nukesort()' to
   // keep track of which is the source and which is the
   // destination. It has to be initialized to 0 so that
   // sorted elements end in 'data' and not in 'buffer'.
   args.b      = 0;
   args.thread = 0;
   args.repeats = 0;

   // Allocate a number of threads that is a power of 2.
   while ((thrmax >> (args.thread + 1)) > 0) args.thread++;

   nukesort(&args);

   free(buffer);
   return numels - args.repeats;

}

void *
nukesort
(
 void * args
)
// SYNOPSIS:
//   Recursive part of 'seqsort'. The code of 'nukesort()' is
//   dangerous and should not be reused. It uses a very special
//   sort order designed for starcode, it destroys some elements
//   and sets them to NULL as it sorts. The sort order is based
//   on the sequences and is such that a < b if a is shorter
//   than b or if a has the same length as b and lower lexical
//   order. When a is the same as b, the useq containing b is
//   destroyed (freed) and replaced by NULL.
//
// ARGUMENTS:
//   args: a sortargs_t struct (see private header file).
//
// RETURN:
//   NULL pointer, regardless of the input
//
// SIDE EFFECTS:
//   Sorts the array of 'useq_t' specified in 'args'.
{

   sortargs_t * sortargs = (sortargs_t *) args;
   if (sortargs->size < 2) return NULL;

   // Next level params.
   sortargs_t arg1 = *sortargs, arg2 = *sortargs;
   arg1.size /= 2;
   arg2.size = arg1.size + arg2.size % 2;
   arg2.buf0 += arg1.size;
   arg2.buf1 += arg1.size;
   arg1.b = arg2.b = (arg1.b + 1) % 2;

   // Either run threads or DIY.
   if (arg1.thread) {
      // Decrease one level.
      arg1.thread = arg2.thread = arg1.thread - 1;
      // Create threads.
      pthread_t thread1, thread2;
      if ( pthread_create(&thread1, NULL, nukesort, &arg1) ||
           pthread_create(&thread2, NULL, nukesort, &arg2) ) {
         alert();
         krash();
      }
      // Wait for threads.
      pthread_join(thread1, NULL);
      pthread_join(thread2, NULL);
   }
   else {
      nukesort(&arg1);
      nukesort(&arg2);
   }

   // Separate data and buffer (b specifies which is buffer).
   useq_t ** l = (sortargs->b ? arg1.buf0 : arg1.buf1);
   useq_t ** r = (sortargs->b ? arg2.buf0 : arg2.buf1);
   useq_t ** buf = (sortargs->b ? arg1.buf1 : arg1.buf0);

   int i = 0;
   int j = 0;
   int idx = 0;
   int cmp = 0;
   int repeats = 0;

   // Merge sets
   while (i+j < sortargs->size) {
      // Only NULLs at the end of the buffers.
      if (j == arg2.size || r[j] == NULL) {
         // Right buffer is exhausted. Copy left buffer...
         memcpy(buf+idx, l+i, (arg1.size-i) * sizeof(useq_t *));
         break;
      }
      if (i == arg1.size || l[i] == NULL) {
         // ... or vice versa.
         memcpy(buf+idx, r+j, (arg2.size-j) * sizeof(useq_t *));
         break;
      }
      if (l[i] == NULL && r[j] == NULL) break;

      // Do the comparison.
      useq_t *ul = (useq_t *) l[i];
      useq_t *ur = (useq_t *) r[j];
      int sl = strlen(ul->seq);
      int sr = strlen(ur->seq);
      if (sl == sr) cmp = strcmp(ul->seq, ur->seq);
      else cmp = sl < sr ? -1 : 1;

      if (cmp == 0) {
         // Identical sequences, this is the "nuke" part.
         // Add sequence counts.
         ul->count += ur->count;
         transfer_useq_ids(ul, ur);
         destroy_useq(ur);
         buf[idx++] = l[i++];
         j++;
         repeats++;
      } 
      else if (cmp < 0) buf[idx++] = l[i++];
      else              buf[idx++] = r[j++];

   }

   // Accumulate repeats
   sortargs->repeats = repeats + arg1.repeats + arg2.repeats;

   // Pad with NULLS.
   int offset = sortargs->size - sortargs->repeats;
   memset(buf+offset, 0, sortargs->repeats*sizeof(useq_t *));
   
   return NULL;

}


gstack_t *
read_rawseq
(
   FILE     * inputf,
   gstack_t * uSQ
)
{

   ssize_t nread;
   size_t nchar = M;
   char copy[MAXBRCDLEN];
   char *line = malloc(M * sizeof(char));
   if (line == NULL) {
      alert();
      krash();
   }

   char *seq = NULL;
   int count = 0;
   int lineno = 0;

   while ((nread = getline(&line, &nchar, inputf)) != -1) {
      lineno++;
      if (line[nread-1] == '\n') line[nread-1] = '\0';
      if (sscanf(line, "%s\t%d", copy, &count) != 2) {
         count = 1;
         seq = line;
      }
      else {
         seq = copy;
      }
      size_t seqlen = strlen(seq);
      if (seqlen > MAXBRCDLEN) {
         fprintf(stderr, "max sequence length exceeded (%d)\n",
               MAXBRCDLEN);
         fprintf(stderr, "offending sequence:\n%s\n", seq);
         abort();
      }
      for (size_t i = 0 ; i < seqlen ; i++) {
         if (!valid_DNA_char[(int)seq[i]]) {
            fprintf(stderr, "invalid input\n");
            fprintf(stderr, "offending sequence:\n%s\n", seq);
            abort();
         }
      }
      useq_t *new = new_useq(count, seq, NULL);
      new->nids = 1;
      new->seqid = (void *)(unsigned long)uSQ->nitems+1;
      push(new, &uSQ);
   }

   free(line);
   return uSQ;

}


gstack_t *
read_fasta
(
   FILE     * inputf,
   gstack_t * uSQ,
   output_t outputt
)
{

   ssize_t nread;
   size_t nchar = M;
   char *line = malloc(M * sizeof(char));
   if (line == NULL) {
      alert();
      krash();
   }

   char *header = NULL;
   int lineno = 0;

   int const readh = outputt == NRED_OUTPUT;
   while ((nread = getline(&line, &nchar, inputf)) != -1) {
      lineno++;
      // Strip newline character.
      if (line[nread-1] == '\n') line[nread-1] = '\0';

      if (lineno %2 == 0) {
         size_t seqlen = strlen(line);
         if (seqlen > MAXBRCDLEN) {
            fprintf(stderr, "max sequence length exceeded (%d)\n",
                  MAXBRCDLEN);
            fprintf(stderr, "offending sequence:\n%s\n", line);
            abort();
         }
         for (size_t i = 0 ; i < seqlen ; i++) {
            if (!valid_DNA_char[(int)line[i]]) {
               fprintf(stderr, "invalid input\n");
               fprintf(stderr, "offending sequence:\n%s\n", line);
               abort();
            }
         }
         useq_t *new = new_useq(1, line, header);
         new->nids = 1;
         new->seqid = (void *)(unsigned long)uSQ->nitems+1;
         if (new == NULL) {
            alert();
            krash();
         }
         push(new, &uSQ);
      }
      else if (readh) {
         header = strdup(line);
         if (header == NULL) {
            alert();
            krash();
         }
      }
   }

   free(line);
   return uSQ;

}


gstack_t *
read_fastq
(
   FILE     * inputf,
   gstack_t * uSQ,
   output_t outputt
)
{

   ssize_t nread;
   size_t nchar = M;
   char *line = malloc(M * sizeof(char));
   if (line == NULL) {
      alert();
      krash();
   }

   char seq[M+1] = {0};
   char header[M+1] = {0};
   char info[2*M+2] = {0};
   int lineno = 0;

   int const readh = outputt == NRED_OUTPUT;
   while ((nread = getline(&line, &nchar, inputf)) != -1) {
      lineno++;
      // Strip newline character.
      if (line[nread-1] == '\n') line[nread-1] = '\0';

      if (readh && lineno % 4 == 1) {
         strncpy(header, line, M);
      }
      else if (lineno % 4 == 2) {
         size_t seqlen = strlen(line);
         if (seqlen > MAXBRCDLEN) {
            fprintf(stderr, "max sequence length exceeded (%d)\n",
                  MAXBRCDLEN);
            fprintf(stderr, "offending sequence:\n%s\n", line);
            abort();
         }
         for (size_t i = 0 ; i < seqlen ; i++) {
            if (!valid_DNA_char[(int)line[i]]) {
               fprintf(stderr, "invalid input\n");
               fprintf(stderr, "offending sequence:\n%s\n", line);
               abort();
            }
         }
         strncpy(seq, line, M);
      }
      else if (lineno % 4 == 0) {
         if (readh) {
            int status = snprintf(info, 2*M+2, "%s\n%s", header, line);
            if (status < 0 || status > 2*M - 1) {
               alert();
               krash();
            }
         }
         useq_t *new = new_useq(1, seq, info);
         new->nids = 1;
         new->seqid = (void *)(unsigned long)uSQ->nitems+1;
         if (new == NULL) {
            alert();
            krash();
         }
         push(new, &uSQ);
      }
   }

   free(line);
   return uSQ;

}


gstack_t *
read_PE_fastq
(
   FILE     * inputf1,
   FILE     * inputf2,
   gstack_t * uSQ,
   output_t outputt
)
{

   char c1 = fgetc(inputf1);
   char c2 = fgetc(inputf2);
   if (c1 != '@' || c2 != '@') {
      fprintf(stderr, "input not a pair of fastq files\n");
      abort();
   }
   if (ungetc(c1, inputf1) == EOF || ungetc(c2, inputf2) == EOF) {
      alert();
      krash();
   }

   ssize_t nread;
   size_t nchar = M;
   char *line1 = malloc(M * sizeof(char));
   char *line2 = malloc(M * sizeof(char));
   if (line1 == NULL && line2 == NULL) {
      alert();
      krash();
   }

   char seq1[M] = {0};
   char seq2[M] = {0};
   char seq[2*M+8] = {0};
   char header1[M] = {0};
   char header2[M] = {0};
   char info[4*M] = {0};
   int lineno = 0;

   int const readh = outputt == NRED_OUTPUT;
   char sep[STARCODE_MAX_TAU+2] = {0};
   memset(sep, '-', STARCODE_MAX_TAU+1);

   while ((nread = getline(&line1, &nchar, inputf1)) != -1) {
      lineno++;
      // Strip newline character.
      if (line1[nread-1] == '\n') line1[nread-1] = '\0';

      // Read line from second file and strip newline.
      if ((nread = getline(&line2, &nchar, inputf2)) == -1) {
         fprintf(stderr, "non conformable paired-end fastq files\n");
         abort();
      }
      if (line2[nread-1] == '\n') line2[nread-1] = '\0';

      if (readh && lineno % 4 == 1) {
         // No check that the headers match each other. At the
         // time of this writing, there are already different
         // formats to link paired-end record. We assume that
         // the users know what they do.
         strncpy(header1, line1, M);
         strncpy(header2, line2, M);
      }
      else if (lineno % 4 == 2) {
         size_t seqlen1 = strlen(line1);
         size_t seqlen2 = strlen(line2);
         if (seqlen1 > MAXBRCDLEN || seqlen2 > MAXBRCDLEN) {
            fprintf(stderr, "max sequence length exceeded (%d)\n",
                  MAXBRCDLEN);
            fprintf(stderr, "offending sequences:\n%s\n%s\n",
                  line1, line2);
            abort();
         }
         for (size_t i = 0 ; i < seqlen1 ; i++) {
            if (!valid_DNA_char[(int)line1[i]]) {
               fprintf(stderr, "invalid input\n");
               fprintf(stderr, "offending sequence:\n%s\n", line1);
               abort();
            }
         }
         for (size_t i = 0 ; i < seqlen2 ; i++) {
            if (!valid_DNA_char[(int)line2[i]]) {
               fprintf(stderr, "invalid input\n");
               fprintf(stderr, "offending sequence:\n%s\n", line2);
               abort();
            }
         }
         strncpy(seq1, line1, M);
         strncpy(seq2, line2, M);
      }
      else if (lineno % 4 == 0) {
         if (readh) {
            int scheck = snprintf(info, 4*M, "%s\n%s\n%s\n%s",
                  header1, line1, header2, line2);
            if (scheck < 0 || scheck > 4*M-1) {
               alert();
               krash();
            }
         }
         else {
            // No need for the headers, the 'info' member is
            // used to hold a string representation of the pair.
            int scheck = snprintf(info, 2*M, "%s/%s", seq1, seq2);
            if (scheck < 0 || scheck > 2*M-1) {
               alert();
               krash();
            }
         }
         int scheck = snprintf(seq, 2*M+8, "%s%s%s", seq1, sep, seq2);
         if (scheck < 0 || scheck > 2*M+7) {
            alert();
            krash();
         }
         useq_t *new = new_useq(1, seq, info);
         new->nids = 1;
         new->seqid = (void *)(unsigned long)uSQ->nitems+1;
         if (new == NULL) {
            alert();
            krash();
         }
         push(new, &uSQ);
      }
   }

   free(line1);
   free(line2);
   return uSQ;

}


gstack_t *
read_file
(
   FILE      * inputf1,
   FILE      * inputf2,
   const int   verbose,
   output_t outputt
)
{

   if (inputf2 != NULL) FORMAT = PE_FASTQ;
   else {
      // Read first line of the file to guess format.
      // Store in global variable FORMAT.
      char c = fgetc(inputf1);
      switch(c) {
         case EOF:
            // Empty file.
            return NULL;
         case '>':
            FORMAT = FASTA;
            if (verbose) fprintf(stderr, "FASTA format detected\n");
            break;
         case '@':
            FORMAT = FASTQ;
            if (verbose) fprintf(stderr, "FASTQ format detected\n");
            break;
         default:
            FORMAT = RAW;
            if (verbose) fprintf(stderr, "raw format detected\n");
      }

      if (ungetc(c, inputf1) == EOF) {
         alert();
         krash();
      }
   }

   gstack_t *uSQ = new_gstack();
   if (uSQ == NULL) {
      alert();
      krash();
   }

   if (FORMAT == RAW)      return read_rawseq(inputf1, uSQ);
   if (FORMAT == FASTA)    return read_fasta(inputf1, uSQ, outputt);
   if (FORMAT == FASTQ)    return read_fastq(inputf1, uSQ, outputt);
   if (FORMAT == PE_FASTQ) return read_PE_fastq(inputf1, inputf2, uSQ, outputt);

   return NULL;

}


int
pad_useq
(
   gstack_t * useqS,
   int      * median
)
{

   // Compute maximum length.
   int maxlen = 0;
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      if (len > maxlen) maxlen = len;
   }

   // Alloc median bins. (Initializes to 0)
   int  * count = calloc((maxlen + 1), sizeof(int));
   char * spaces = malloc((maxlen + 1) * sizeof(char));
   if (spaces == NULL || count == NULL) {
      alert();
      krash();
   }
   for (int i = 0 ; i < maxlen ; i++) spaces[i] = ' ';
   spaces[maxlen] = '\0';

   // Pad all sequences with spaces.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      count[len]++;
      if (len == maxlen) continue;
      // Create a new sequence with padding characters.
      char *padded = malloc((maxlen + 1) * sizeof(char));
      if (padded == NULL) {
         alert();
         krash();
      }
      memcpy(padded, spaces, maxlen + 1);
      memcpy(padded+maxlen-len, u->seq, len);
      free(u->seq);
      u->seq = padded;
   }

   // Compute median.
   *median = 0;
   int ccount = 0;
   do {
      ccount += count[++(*median)];
   } while (ccount < useqS->nitems / 2);

   // Free and return.
   free(count);
   free(spaces);
   return maxlen;

}


void
unpad_useq
(
   gstack_t *useqS
)
{
   // Take the length of the first sequence (assume all
   // sequences have the same length).
   int len = strlen(((useq_t *) useqS->items[0])->seq);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = (useq_t *) useqS->items[i];
      int pad = 0;
      while (u->seq[pad] == ' ') pad++;
      // Create a new sequence without paddings characters.
      char *unpadded = malloc((len - pad + 1) * sizeof(char));
      if (unpadded == NULL) {
         alert();
         krash();
      }
      memcpy(unpadded, u->seq + pad, len - pad + 1);
      free(u->seq);
      u->seq = unpadded;
   }
   return;
}

void
transfer_useq_ids
(
 useq_t * ud,
 useq_t * us
)
// Appends the sequence ID list from 'us' to 'ud'.
// If 'ud' is only bearing one ID, the function
// will allocate a buffer on 'ud->seqid'. Otherwise
// the ID borne by 'us' will be appended to the
// existing buffer.
// The sequence ID list from ud is not modified.
{
   if (us->nids < 1) return;
   // Alloc buffer.
   int * buf = malloc((ud->nids + us->nids) * sizeof(int));
   if (buf == NULL) {
      alert();
      krash();
   }
   int * s, * d;
   if (ud->nids > 1) d = ud->seqid;
   else d = (int *)&(ud->seqid);
   if (us->nids > 1) s = us->seqid;
   else s = (int *)&(us->seqid);
   // Merge algorithm (keeps them sorted and unique).
   uint32_t i = 0, j = 0, k = 0;
   while (i < ud->nids && j < us->nids) {
      if (d[i] < s[j])
         buf[k++] = d[i++];
      else if (d[i] > s[j])
         buf[k++] = s[j++];
      else {
         buf[k++] = d[i];
         i++; j++;
      }
   }
   // TODO: use memcpy.
   for (; i < ud->nids; i++) buf[k++] = d[i];
   for (; j < us->nids; j++) buf[k++] = s[j];
   // Update ID count.
   if (ud->nids > 1) free(ud->seqid);
   ud->seqid = buf;
   ud->nids = k;
}


void
transfer_counts_and_update_canonicals
(
 useq_t *useq,
 int     transfer_ids
)
// TODO: Write the doc.
// SYNOPSIS:
//   Function used in message passing clustering.
{
   // If the read has no matches, it has no parent, so
   // it is an ancestor and it must be canonical.
   if (useq->matches == NULL) {
      useq->canonical = useq;
      return;
   }

   // If the read has already been assigned a canonical, directly
   // transfer counts and ids to the canonical and return.
   if (useq->canonical != NULL) {
      useq->canonical->count += useq->count;
      if (transfer_ids)
         transfer_useq_ids(useq->canonical, useq);
      // Counts transferred, remove from self.
      useq->count = 0;
      return;
   }

   // The field 'matches' stores parents (useq with higher
   // counts) stratified by distance to self.
   // Consider that direct parents are the lowest nonempty
   // match stratum. Others are disregarded (indirect).
   gstack_t *matches;
   for (int i = 0 ; (matches = useq->matches[i]) != TOWER_TOP ; i++) {
      if (matches->nitems > 0) break;
   }

   // Distribute counts evenly among parents.
   int Q = useq->count / matches->nitems;
   int R = useq->count % matches->nitems;
   // Depending on the order in which matches were made
   // (which is more or less random), the transferred
   // counts can differ by 1. For this reason, the output
   // can be slightly different for different number of tries.
   for (int i = 0 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      match->count += Q + (i < R);
      // transfer sequence ID to all the matches.
      if (transfer_ids)
         transfer_useq_ids(match, useq);
   }
   // Counts transferred, remove from self.
   useq->count = 0;

   // Continue propagation to direct parents. This will update
   // the canonicals of the whole ancestry.
   for (int i = 0 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      transfer_counts_and_update_canonicals(match, transfer_ids);
   }

   // Self canonical is the canonical of the first parent...
   useq_t *canonical = ((useq_t *) matches->items[0])->canonical;
   // ... but if parents have different canonicals then
   // self canonical is set to 'NULL'.
   for (int i = 1 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      if (match->canonical != canonical) {
         canonical = NULL;
         break;
      }
   }

   useq->canonical = canonical;

   return;

}


int
addmatch
(
   useq_t * to,
   useq_t * from,
   int      dist,
   int      maxtau
)
// SYNOPSIS:
//   Add a sequence to the match record of another.
//
// ARGUMENTS:
//   to: pointer to the sequence to add the match to
//   from: pointer to the sequence to add as a match
//   dist: distance between the sequences
//   maxtau: maximum allowed distance
//
// RETURN:
//   0 upon success, 1 upon failure.
//
// SIDE EFFECT:
//   Updates sequence pointed to by 'to' in place, potentially
//   creating a match record,
{

   // Cannot add a match at a distance greater than 'maxtau'
   // (this lead to a segmentation fault).
   if (dist > maxtau) return 1;
   // Create stack if not done before.
   if (to->matches == NULL) to->matches = new_tower(maxtau+1);
   return push(from, to->matches + dist);

}


lookup_t *
new_lookup
(
 int slen,
 int maxlen,
 int tau
)
{

   lookup_t * lut = (lookup_t *) malloc(2*sizeof(int) + sizeof(int *) +
         (tau+1)*sizeof(char *));
   if (lut == NULL) {
      alert();
      return NULL;
   }

   // Target size.
   int k   = slen / (tau + 1);
   int rem = tau - slen % (tau + 1);

   // Set parameters.
   lut->slen  = maxlen;
   lut->kmers = tau + 1;
   lut->klen  = malloc(lut->kmers * sizeof(int));
   
   // Compute k-mer lengths.
   if (k > MAX_K_FOR_LOOKUP)
      for (int i = 0; i < tau + 1; i++) lut->klen[i] = MAX_K_FOR_LOOKUP;
   else
      for (int i = 0; i < tau + 1; i++) lut->klen[i] = k - (rem-- > 0);

   // Allocate lookup tables.
   for (int i = 0; i < tau + 1; i++) {
      lut->lut[i] = calloc(1 << max(0,(2*lut->klen[i] - 3)),
            sizeof(char));
      if (lut->lut[i] == NULL) {
         while (--i >= 0) {
            free(lut->lut[i]);
         }  
         free(lut);
         alert();
         return NULL;
      }
   }

   return lut;

}


void
destroy_lookup
(
   lookup_t * lut
)
{
   for (int i = 0 ; i < lut->kmers ; i++) free(lut->lut[i]);
   free(lut->klen);
   free(lut);
}


int
lut_search
(
 lookup_t * lut,
 useq_t   * query
)
// SYNOPSIS:
//   Perform of a lookup search of the query and determine whether
//   at least one of the k-mers extracted from the query was inserted
//   in the lookup table. If this is not the case, the trie search can
//   be skipped because the query cannot have a match for the given
//   tau.
//   
// ARGUMENTS:
//   lut: the lookup table to search
//   query: the query as a useq.
//
// RETURN:
//   1 if any of the k-mers extracted from the query is in the
//   lookup table, 0 if not, and -1 in case of failure.
//
// SIDE-EFFECTS:
//   None.
{

   // Start from the end of the sequence. This will avoid potential
   // misalignments on the first kmer due to insertions.
   int offset = lut->slen;
   // Iterate for all k-mers and for ins/dels.
   for (int i = lut->kmers - 1; i >= 0; i--) {
      offset -= lut->klen[i];
      for (int j = -(lut->kmers - 1 - i); j <= lut->kmers - 1 - i; j++) {
         // If sequence contains 'N' seq2id will return -1.
         int seqid = seq2id(query->seq + offset + j, lut->klen[i]);
         // Make sure to never proceed passed the end of string.
         if (seqid == -2) return -1;
         if (seqid == -1) continue;
         // The lookup table proper is implemented as a bitmap.
         if ((lut->lut[i][seqid/8] >> (seqid%8)) & 1) return 1;
      }
   }

   return 0;

}


int
lut_insert
(
   lookup_t * lut,
   useq_t   * query
)
{

   int seqlen = strlen(query->seq);

   int offset = lut->slen;
   for (int i = lut->kmers-1; i >= 0; i--) {
      offset -= lut->klen[i];
      if (offset + lut->klen[i] > seqlen) continue;
      int seqid = seq2id(query->seq + offset, lut->klen[i]);
      // The lookup table proper is implemented as a bitmap.
      if (seqid >= 0) lut->lut[i][seqid/8] |= (1 << (seqid%8));
      // Make sure to never proceed passed the end of string.
      else if (seqid == -2) return 1;
   }

   // Insert successful.
   return 0;

}


int
seq2id
(
  char * seq,
  int    slen
)
{

   int seqid = 0;
   for (int i = 0; i < slen; i++) {
      // Padding spaces are substituted by 'A'. It does not hurt
      // anyway to generate some false positives.
      if      (seq[i] == 'A' || seq[i] == 'a' || seq[i] == ' ') { }
      else if (seq[i] == 'C' || seq[i] == 'c') seqid += 1;
      else if (seq[i] == 'G' || seq[i] == 'g') seqid += 2;
      else if (seq[i] == 'T' || seq[i] == 't') seqid += 3;
      // Non DNA character (including end of string).
      else return seq[i] == '\0' ? -2 : -1;
      if (i < slen - 1) seqid <<= 2;
   }

   return seqid;

}


useq_t *
new_useq
(
   int    count,
   char * seq,
   char * info
)
{
   // Check input.
   if (seq == NULL) return NULL;

   useq_t *new = calloc(1, sizeof(useq_t));
   if (new == NULL) {
      alert();
      krash();
   }
   size_t slen = strlen(seq);
   new->seq = malloc(slen+1);
   for (size_t i = 0; i < slen; i++)
      new->seq[i] = capitalize[(uint8_t)seq[i]];
   new->seq[slen] = 0;
   new->count = count;
   new->nids  = 0;
   new->seqid = NULL;
   if (info != NULL) {
      new->info = strdup(info);
      if (new->info == NULL) {
         alert();
         krash();
      }
   }

   return new;

}


void
destroy_useq
(
   useq_t *useq
)
{
   if (useq->matches != NULL) destroy_tower(useq->matches);
   if (useq->info != NULL) free(useq->info);
   if (useq->nids > 1) free(useq->seqid);
   free(useq->seq);
   free(useq);
}


int
canonical_order
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = *((useq_t **) a);
   useq_t *u2 = *((useq_t **) b);
   if (u1->canonical == u2->canonical) return strcmp(u1->seq, u2->seq);
   if (u1->canonical == NULL) return 1;
   if (u2->canonical == NULL) return -1;
   if (u1->canonical->count == u2->canonical->count) {
      return strcmp(u1->canonical->seq, u2->canonical->seq);
   }
   if (u1->canonical->count > u2->canonical->count) return -1;
   return 1;
} 


int
count_order
(
 const void *a,
   const void *b
 )
{
   useq_t *u1 = *((useq_t **) a);
   useq_t *u2 = *((useq_t **) b);
   if (u1->count == u2->count) return strcmp(u1->seq, u2->seq);
   else return u1->count < u2->count ? 1 : -1;
} 


int
count_order_spheres
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = *((useq_t **) a);
   useq_t *u2 = *((useq_t **) b);
   // Same count, sort by cluster size.
   if (u1->count == u2->count) {
      if (u2->matches == NULL) return -1;
      if (u1->matches == NULL) return 1;

      // This counts the whole cluster size because the edge
      // references are bidirectional in spheres clustering.
      long cnt1 = 0, cnt2 = 0;
      gstack_t *matches;
      for (int j = 0 ; (matches = u1->matches[j]) != TOWER_TOP ; j++)
         for (int k = 0; k < matches->nitems; k++)
            cnt1 += ((useq_t *)matches->items[k])->count;

      for (int j = 0 ; (matches = u2->matches[j]) != TOWER_TOP ; j++)
         for (int k = 0; k < matches->nitems; k++)
            cnt2 += ((useq_t *)matches->items[k])->count;

      return cnt1 < cnt2 ? 1 : -1;
   }
   else return u1->count < u2->count ? 1 : -1;
} 

int
cluster_count
(
 const void *a,
 const void *b
)
{
   gstack_t *s1 = *((gstack_t **) a);
   gstack_t *s2 = *((gstack_t **) b);
   if (((useq_t *) s1->items[0])->count < ((useq_t *) s2->items[0])->count)
      return 1;
   else
      return -1;
}

int
int_ascending
(
 const void *a,
 const void *b
)
{
   if (*(int *)a < *(int *)b) return -1;
   else return 1;
}


void
krash
(void)
{
   fprintf(stderr,
      "starcode has crashed, please contact guillaume.filion@gmail.com "
      "for support with this issue.\n");
   abort();
}
