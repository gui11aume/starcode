#include "starcode.h"

#define MAXPAR 64

char *USAGE = "Usage:\n"
"  tquery [-v] [-d distance] [-i index] [-q query] [-o output]\n"
"    -v: verbose\n"
"    -d: maximum Levenshtein distance\n"
"    -i: index file\n"
"    -q: query file (default stdin)\n"
"    -o: output file (default stdout)\n";

void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }



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


int
tquery
(
   FILE *indexf,
   FILE *queryf,
   FILE *outputf,
   // control //
   int maxmismatch,
   const int verbose
)
{

   // -- STEP 1: set up the index -- //
   trienode *index = new_trienode();
   if (index == NULL) exit(EXIT_FAILURE);

   // Read the data from input file. We assume that it contains
   // one barcode per line and nothing else. 
   char *barcode = malloc(MAXBRCDLEN * sizeof(char));
   size_t nchar = MAXBRCDLEN;
   ssize_t read;
   if (verbose) fprintf(stderr, "constructing index\n");
   while ((read = getline(&barcode, &nchar, indexf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';

      // Add barcode to index.
      trienode *node = insert_string(index, barcode);
      if (node == NULL) exit(EXIT_FAILURE);
      node->data = node;
   }
   
   // -- STEP 2: collect unique queries -- //
   // Insertion is faster than search. We use it to reduce the
   // volume of identical queries.
   trienode *counter = new_trienode();
   if (counter == NULL) exit(EXIT_FAILURE);

   int size = 1024;
   star_t **stars = malloc(size * sizeof(star_t *));

   int total = 0;
   while ((read = getline(&barcode, &nchar, queryf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';
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

   hip_t *hip = new_hip();
   if (hip == NULL) exit(EXIT_FAILURE);

   // -- STEP 3: query index with unique barcodes -- //
   for (int i = 0 ; i < total ; i++) {
      // Query barcode in index.
      clear_hip(hip);
      search(index, stars[i]->barcode, maxmismatch, hip);
      fprintf(outputf, "%s\t%d\t", stars[i]->barcode, stars[i]->count);

      if (hip_error(hip)) {
         fprintf(outputf, "!\n");
         continue;
      }

      if (hip->n_hits == 0) {
         fprintf(outputf, "-\n");
         continue;
      }

      // No error and there is a hit.
      fprintf(outputf, "%s", hip->hits[0].match);
      for (int i = 1 ; i < hip->n_hits ; i++) {
         fprintf(outputf, ",%s", hip->hits[i].match);
      }
      fprintf(outputf, "\n");
      if (verbose) fprintf(stderr, "tquery: %d/%d\r", i, total);
   }
   if (verbose) fprintf(stderr, "\n");

   // -- STEP 4: clean up -- //
   destroy_hip(hip);
   destroy_nodes_downstream_of(index, NULL);
   for (int i = 0 ; i < total ; i++) {
      free(stars[i]->barcode);
      free(stars[i]);
   }
   free(stars);
   free(barcode);

   return 0;

}

// By default tquery has a `main()` except if compiled with
// flag -D_no_starcode_main (for testing).
#ifndef _no_tquery_main
int
main(
   int argc,
   char **argv
)
{
   int vflag = 0;
   int dflag = 3;
   char *index = NULL;
   char *query = NULL;
   char *output = NULL;
   int c;

   while ((c = getopt(argc, argv, "vd:i:q:o:")) != -1) {
      switch (c) {
        case 'v':
          vflag = 1;
          break;
        case 'd':
          dflag = atoi(optarg);
          break;
        case 'i':
          index = optarg;
          break;
        case 'q':
          query = optarg;
          break;
        case 'o':
          output = optarg;
          break;
        case '?':
          if (optopt=='d' || optopt=='i' || optopt == 'q' || optopt=='o')
            fprintf(stderr, "option -%c requires an argument\n", optopt);
          else if (isprint(optopt))
            fprintf(stderr, "unknown option `-%c'.\n", optopt);
          else
            fprintf(stderr, "unknown option `\\x%x'.\n", optopt);
          return 1;
        default:
          abort();
      }
   }

   if (optind < argc) {
      fprintf(stderr, "too many options\n");
      say_usage();
      return 1;
   }

   FILE *indexf;
   FILE *queryf;
   FILE *outputf;

   if (index == NULL) {
      fprintf(stderr, "no index file\n");
      say_usage();
      return 1;
   }

   indexf = fopen(index, "r");
   if (indexf == NULL) {
      fprintf(stderr, "cannot open file %s\n", index);
      say_usage();
      return 1;
   }

   if (query != NULL) {
      queryf = fopen(query, "r");
      if (queryf == NULL) {
         fprintf(stderr, "cannot open file %s\n", query);
         say_usage();
         return 1;
      }
   }
   else {
      queryf = stdin;
   }

   if (output != NULL) {
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

   int exitcode = tquery(indexf, queryf, outputf, dflag, vflag);
   
   fclose(indexf);
   fclose(queryf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;
}
#endif
