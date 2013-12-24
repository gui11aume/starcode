#include "starcode.h"

char *USAGE = "Usage:\n"
"  starcode [-v] [-d dist] [-i input] [-o output]\n"
"    -v --verbose: verbose\n"
"    -d --dist: maximum Levenshtein distance\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)";

#define str(a) *(char **)(a)

useq_t *new_useq(int, char*);
void destroy_useq (void*);
void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }
int abcd(const void *a, const void *b) { return strcmp(str(a), str(b)); }

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   const int maxmismatch,
   const int verbose
)
{

   size_t size = 1024;
   char **all_seq = malloc(size * sizeof(char *));
   if (all_seq == NULL) return 1;

   int total = 0;
   ssize_t nread;
   size_t nchar = MAXBRCDLEN;
   char *seq = malloc(MAXBRCDLEN * sizeof(char));
   // Read sequences from input file and store in an array. Assume
   // that it contains one sequence per line and nothing else. 
   if (verbose) fprintf(stderr, "reading input file...");
   while ((nread = getline(&seq, &nchar, inputf)) != -1) {
      // Strip end of line character and copy.
      if (seq[nread-1] == '\n') seq[nread-1] = '\0';
      char *new = malloc(nread);
      if (new == NULL) exit(EXIT_FAILURE);
      strncpy(new, seq, nread);
      // Grow 'all_seq' if needed.
      if (total >= size) {
         size *= 2;
         char **ptr = realloc(all_seq, size * sizeof(char *));
         if (ptr == NULL) exit(EXIT_FAILURE);
         all_seq = ptr;
      }
      all_seq[total++] = new;
   }
      
   free(seq);
   if (verbose) fprintf(stderr, " done\n");

   if (verbose) fprintf(stderr, "sorting sequences...");
   // Sort sequences, count and compact them. Sorting
   // alphabetically is important for speed.
   qsort(all_seq, total, sizeof(char *), abcd);

   size = 1024;
   useq_t **all_useq = malloc(size * sizeof(useq_t *));
   int utotal = 0;
   int ucount = 0;
   for (int i = 0 ; i < total-1 ; i++) {
      ucount++;
      if (strcmp(all_seq[i], all_seq[i+1]) == 0) free(all_seq[i]);
      else {
         all_useq[utotal++] = new_useq(ucount, all_seq[i]);
         ucount = 0;
         // Grow 'all_useq' if needed.
         if (utotal >= size) {
            size *= 2;
            useq_t **ptr = realloc(all_useq, size * sizeof(useq_t *));
            if (ptr == NULL) exit(EXIT_FAILURE);
            all_useq = ptr;
         }
      }
   }
   all_useq[utotal++] = new_useq(ucount+1, all_seq[total-1]);

   free(all_seq);
   if (verbose) fprintf(stderr, " done\n");

   node_t *trie = new_trienode();
   narray_t *hits = new_narray();
   if (trie == NULL || hits == NULL) exit(EXIT_FAILURE);

   for (int i = 0 ; i < utotal ; i++) {
      useq_t *query = all_useq[i];
      // Clear hits.
      hits->idx = 0;
      hits = search(trie, query->seq, maxmismatch, hits);
      if (check_trie_error_and_reset()) {
         fprintf(stderr, "search error: %s\n", query->seq);
         continue;
      }

      // Output matching pairs.
      for (int j = 0 ; j < hits->idx ; j++) {
         useq_t *match = (useq_t *)hits->nodes[j]->data;
         useq_t *u1 = query;
         useq_t *u2 = match;
         if (u1->count < u2->count) {
            u2 = query;
            u1 = match;
         }
         else if (u1->count == u2->count && strcmp(u1->seq, u2->seq) > 0) {
            u2 = query;
            u1 = match;
         }
         fprintf(outputf, "%s:%d\t%s:%d\n",
               u1->seq, u1->count, u2->seq, u2->count);
      }

      // Insert the new barcode in the trie.
      node_t *node = insert_string(trie, query->seq);
      if (node == NULL) exit(EXIT_FAILURE);
      node->data = all_useq[i];
      if (verbose) fprintf(stderr, "starcode: %d/%d\r", i+1, utotal);
   }

   free(all_useq);
   destroy_nodes_downstream_of(trie, destroy_useq);

   if (verbose) fprintf(stderr, "\n");

   return 0;

}


useq_t *
new_useq
(
   int count,
   char *seq
)
{
   useq_t *new = malloc(sizeof(useq_t));
   if (new == NULL) exit(EXIT_FAILURE);
   new->count = count;
   new->seq = seq;
   return new;
}


void
destroy_useq
(
   void *u
)
{
   useq_t *useq = (useq_t *)u;
   free(useq->seq);
   free(useq);
}


// By default starcode has a `main()` except if compiled with
// flag -D_no_starcode_main (for testing).
#ifndef _no_starcode_main
int
main(
   int argc,
   char **argv
)
{

   // Unset flags (value -1).
   int verbose_flag = -1;
   int format_flag = -1;
   int dist_flag = -1;
   // Unset options (value 'UNSET').
   char _u; char *UNSET = &_u;
   char *format_option = UNSET;
   char *input = UNSET;
   char *output = UNSET;

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"verbose", no_argument,       0, 'v'},
         {"help",    no_argument,       0, 'h'},
         {"format",  required_argument, 0, 'f'},
         {"dist",    required_argument, 0, 'd'},
         {"input",   required_argument, 0, 'i'},
         {"output",  required_argument, 0, 'o'},
         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "d:i:f:ho:v",
            long_options, &option_index);
 
      /* Detect the end of the options. */
      if (c == -1) break;
  
      switch (c) {
         case 'd':
            if (dist_flag < 0) {
               dist_flag = atoi(optarg);
            }
            else {
               fprintf(stderr, "distance option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 0:
         case 'i':
            if (input == UNSET) {
               input = optarg;
            }
            else {
               fprintf(stderr, "input option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'f':
            if (format_option == UNSET) {
               if (strcmp(optarg, "counts") == 0) {
                  format_flag = 2;
               }
               else if (strcmp(optarg, "graph") == 0) {
                  format_flag = 0;
               }
               else if (strcmp(optarg, "both") == 0) {
                  format_flag = 1;
               }
               else {
                  fprintf(stderr, "invalid format option\n");
                  say_usage();
                  return 1;
               }
               format_option = optarg;
            }
            else {
               fprintf(stderr, "format option set more than once\n");
               say_usage();
               return 1;
            }
            break;
  
         case 'o':
            if (output == UNSET) {
               input = optarg;
            }
            else {
               fprintf(stderr, "output option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'v':
            if (verbose_flag < 0) {
               verbose_flag = 1;
            }
            else {
               fprintf(stderr, "verbose option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'h':
            say_usage();
            return 0;
 
         default:
            say_usage();
            return 1;

      }

   }

   if (optind < argc) {
      if ((optind == argc - 1) && (input == UNSET)) {
         input = argv[optind];
      }
      else {
         fprintf(stderr, "too many options\n");
         say_usage();
         return 1;
      }
   }

   FILE *inputf;
   FILE *outputf;

   if (input != UNSET) {
      inputf = fopen(input, "r");
      if (inputf == NULL) {
         fprintf(stderr, "cannot open file %s\n", input);
         say_usage();
         return 1;
      }
   }
   else {
      inputf = stdin;
   }

   if (output != UNSET) {
      outputf = fopen(output, "w");
      if (outputf == NULL) {
         fprintf(stderr, "cannot write to file %s\n", output);
         say_usage();
         return 1;
      }
   }
   else {
      outputf = stdout;
   }

   if (verbose_flag < 0) verbose_flag = 0;
   if (format_flag < 0) format_flag = 2;
   if (dist_flag < 0) dist_flag = 3;

   int exitcode = starcode(inputf, outputf, dist_flag, verbose_flag); 

   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;

}
#endif
