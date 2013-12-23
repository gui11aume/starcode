#include "starcode.h"

char *USAGE = "Usage:\n"
"  starcode [-v] [-f fmt] [-d dist] [-i input] [-o output]\n"
"    -v --verbose: verbose\n"
"    -f --format: counts (default), graph, both\n"
"    -d --dist: maximum Levenshtein distance\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)";

void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }

FILE *OUTPUT = NULL;

char *
node_to_str
(
   node_t *node,
   char *buff
)
{
   buff[node->depth] = '\0';
   for (int i = node->depth-1 ; node->parent != NULL ; i--) {
      buff[i] = untranslate[node->pos];
      node = node->parent;
   }
   return buff;
}


void print_pairs
(
   node_t *node
)
{
   if (node->data == NULL) return;
   char str1[M];
   char str2[M];
   for (int i = 0 ; i < node->as->idx ; i++) {
      node_t *active_node = node->as->node[i];
      if (active_node->data == NULL) continue;
      node_t *n1 = node;
      node_t *n2 = active_node;
      if (*(int *)active_node->data > *(int *)node->data) {
         n1 = active_node;
         n2 = node;
      }
      fprintf(OUTPUT, "%s:%d\t%s:%d\n",
            node_to_str(n1, str1), *(int *)n1->data,
            node_to_str(n2, str2), *(int *)n2->data
      );
   }
}


int
starcode
(
   FILE *inputf,
   FILE *outputf,
   // control //
   const int maxmismatch,
   const int verbose
)
{

   OUTPUT = outputf;

   // Build a set of barcodes, associated with their count.
   node_t *trie = new_trienode();
   if (trie == NULL) exit(EXIT_FAILURE);
   
   //int size = 1024;
   //star_t **stars = malloc(size * sizeof(star_t *));

   // Read the data from input file. We assume that it contains
   // one barcode per line and nothing else. 
   //int total = 0;
   char *barcode = malloc(MAXBRCDLEN * sizeof(char));
   size_t nchar = MAXBRCDLEN;
   ssize_t read;
   if (verbose) fprintf(stderr, "building trie index...");
   while ((read = getline(&barcode, &nchar, inputf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';

      // Add barcode to counter.
      node_t *node = insert_string(trie, barcode);
      if (node->data == NULL) {
         node->data = malloc(sizeof(int));
      }
      *(int *)node->data += 1;
   }
   if (verbose) fprintf(stderr, " done\n");

   // Destroy the trie, but keep the star_t and sort them.
   //destroy_nodes_downstream_of(counter, NULL);
   //qsort(stars, total, sizeof(star_t *), cmpstar);
   triestack(trie, maxmismatch, print_pairs);

   destroy_nodes_downstream_of(trie, free);
   free(barcode);

   OUTPUT = NULL;

   return 0;

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
