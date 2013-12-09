#include "starcode.h"

char *USAGE = "Usage:\n"
"  starcode [-v] [-f fmt] [-d dist] [-i input] [-o output]\n"
"    -v --verbose: verbose\n"
"    -f --format: counts (default), graph, both\n"
"    -d --dist: maximum Levenshtein distance\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)";

void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   // control //
   const int maxmismatch,
   const int format,
   const int verbose
)
{

   // The barcodes first have to be counted. Since we trie
   // implementation at hand, we use it for this purpose.
   trienode *counter = new_trienode();
   if (counter == NULL) exit(EXIT_FAILURE);
   
   int size = 1024;
   star_t **stars = malloc(size * sizeof(star_t *));

   // Read the data from input file. We assume that it contains
   // one barcode per line and nothing else. 
   int total = 0;
   char *barcode = malloc(MAXBRCDLEN * sizeof(char));
   size_t nchar = MAXBRCDLEN;
   ssize_t read;
   while ((read = getline(&barcode, &nchar, inputf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';

      // Add barcode to counter.
      trienode *node = insert_string(counter, barcode);
      if (node->data == NULL) {
         if (total >= size) {
            star_t **ptr = realloc(stars, 2*size * sizeof(star_t *));
            if (ptr == NULL) exit(EXIT_FAILURE);
            size *= 2;
            stars = ptr;
         }
         star_t *h = malloc(sizeof(star_t));
         h->barcode = malloc((1+strlen(barcode)) * sizeof(char));
         strcpy(h->barcode, barcode);
         h->count = 0;
         node->data = stars[total] = h;
         total++;
      }
      (((star_t *)node->data)->count)++;
   }

   // Destroy the trie, but keep the star_t and sort them.
   destroy_nodes_downstream_of(counter, NULL);
   qsort(stars, total, sizeof(star_t *), cmpstar);

   // Every barcode cluster has a canonical representation, which
   // is the most abundant. The other are non canonical.
   trienode *trie = new_trienode();
   hip_t *hip = new_hip();
   if (trie == NULL || hip == NULL) exit(EXIT_FAILURE);

   for (int i = 0 ; i < total ; i++) {

      // Search the with at most 'maxmismatch'.
      char *barcode = stars[i]->barcode;
      search(trie, barcode, maxmismatch, hip);

      if (hip_error(hip)) {
         fprintf(stderr, "search error: %s\n", barcode);
         continue;
      }

      // Insert the new barcode in the trie and update parents.
      trienode *node = insert_string(trie, barcode);
      if (node == NULL) exit(EXIT_FAILURE);

      star_t *star = stars[i];
      node->data = star;

      if (hip->n_hits == 0) { 
         // Barcode is canonical.
         star->parents[0] = star;
         star->parents[1] = NULL;
      }
      else {
         // Barcode is non canonical.
         int j;
         int mindist = hip->hits[0].dist;
         int jmax = hip->n_hits >= MAXPAR ? MAXPAR-1 : hip->n_hits;
         for (j = 0 ; j < jmax ; j++) {
            star->parents[j] = (star_t *) hip->hits[j].node->data;
            if (hip->hits[j].dist > mindist) {
               j++;
               break;
            }
         }
         star->parents[j]= NULL;
      }
      clear_hip(hip);
      if (verbose) fprintf(stderr, "starcode: %d/%d\r", i, total);
   }
   if (verbose) fprintf(stderr, "\n");

   if (format > 0) {
      // In standard output, only print the barcode and the count
      // of all barcodes of the culster (i.e. all the barcodes with
      // the same canonical). Starting from the end, pass counts
      // from children to parents.
      for (int i = total-1 ; i > -1 ; i--) {
         star_t *star = stars[i];
         star_t *parent = star->parents[0];
         if (parent == star) continue;
         float pcount = parent->count;
         // Sum parental counts. Here I don't want to lose counts
         // because of rounding, so I am going through a bit of
         // funny integer arithmetics.
         int piece;
         int cake = star->count;
         for (int j = 1 ; (parent = star->parents[j]) != NULL ; j++) {
            pcount += parent->count;
         }
         // Divide 'cake' among parents in proportion of their own
         // number of counts.
         for (int j = 0 ; (parent = star->parents[j]) != NULL ; j++) {
            parent->count += piece = star->count * parent->count / pcount;
            cake -= piece;
         }
         // Parent of index 0 always gets the last piece of cake if
         // anything is left (because he always exists).
         star->parents[0]->count += cake;
      }

      // Counts have been updated. Sort again and print.
      qsort(stars, total, sizeof(star_t *), cmpstar);

      for (int i = 0 ; i < total ; i++) {
         star_t *this = stars[i];
         if (this->parents[0] != this) continue;
         fprintf(outputf, "%s\t%d\n", this->barcode, this->count);
      }
   }

   if (format == 1) {
      fprintf(outputf, "--\n");
   }

   if (format < 2) {
      // If graph is required, print barcode, count
      // and parents (separated by commas).
      fprintf(outputf, "barcode\tcount\tparents\n");
      for (int i = 0 ; i < total ; i++) {
         star_t this = *(stars[i]);
         fprintf(outputf, "%s\t%d\t%s",
               this.barcode, this.count, this.parents[0]->barcode);
         for (int j = 1 ; this.parents[j] != NULL ; j++) {
            fprintf(outputf, ",%s", this.parents[j]->barcode);
         }
         fprintf(outputf, "\n");
      }
   }

   destroy_nodes_downstream_of(trie, NULL);
   for (int i = 0 ; i < total ; i++) {
      free(stars[i]->barcode);
      free(stars[i]);
   }
   free(stars);
   free(barcode);

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

   int exitcode = starcode(inputf, outputf,
         dist_flag, format_flag, verbose_flag);
   
   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;

}
#endif
