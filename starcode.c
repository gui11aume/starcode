#include "starcode.h"

struct _rel {
   int count;
   char *barcode;
   struct _rel *canonical;
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

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   // control //
   int maxmismatch,
   int compact_output
)
{

   // The barcodes first have to be counted. Since we trie
   // implementation at hand, we use it for this purpose.
   trienode *counter = new_trienode();
   
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

   fprintf(stderr, "\n");
   for (int i = 0 ; i < total ; i++) {

      // Search the index, with 'maxmismatch' allowed mismatches.
      char *barcode = stars[i]->barcode;
      search(trie, barcode, maxmismatch, hip);

      // Insert the new barcode in the trie.
      trienode *node = insert_string(trie, barcode);
      if (node == NULL) exit(EXIT_FAILURE);

      star_t *star = stars[i];
      if (hip->n_hits == 0) { 
         // Barcode is canonical.
         star->canonical = star;
         node->data = star;
      }
      else {
         // Barcode is non canonical.
         star_t *canonical = (star_t *) hip->hits[0].node->data;
         canonical = canonical->canonical;
         int mindist = hip->hits[0].dist;
         for (int j = 1 ; j < hip->n_hits ; j++) {
            hit_t hit = hip->hits[j];
            if (hit.dist > mindist) break;
            if (((star_t *)hit.node->data)->canonical != canonical) {
               // XXX Lame... XXX
               canonical = NULL;
               break;
            }
         }
         star->canonical = canonical;
         if (canonical != NULL) {
            node->data = star;
            if (compact_output) {
               // Transfer counts to canonical.
               canonical->count += star->count;
               star->count = 0;
            }
         }
      }
      clear_hip(hip);
      fprintf(stderr, "%d\r", i);
   }
   fprintf(stderr, "\n");

   if (compact_output) {
      // In compact output, only print the barcode and the count
      // of all barcodes of the culster (i.e. all the barcodes with
      // the same canonical).
      
      qsort(stars, total, sizeof(star_t *), cmpstar);
      for (int i = 0 ; i < total ; i++) {
         star_t *this = stars[i];
         if (this->canonical != this) continue;
         fprintf(outputf, "%s\t%d\n", this->barcode, this->count);
      }
   }
   else {
      // In non compact output, print barcode, canonical and count.
      fprintf(outputf, "barcode\tcanonical\tcount\n");
      // Unlike for compact output there is no need to sort again
      // because the counts have not changed.
      for (int i = 0 ; i < total ; i++) {
         star_t this = *(stars[i]);
         fprintf(outputf, "%s\t%s\t%d\n", this.barcode,
               this.canonical == NULL ? "NA" : this.canonical->barcode,
               this.count);
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
// flag -D_no_starcode_main (e.g. for testing).
#ifndef _no_starcode_main
int
main(
   int argc,
   char **argv
)
{
   int cflag = 0;
   int mflag = 3;
   char *input = NULL;
   char *output = NULL;
   int c;

   while ((c = getopt (argc, argv, "cm:o:i:")) != -1) {
      switch (c) {
        case 'c':
          cflag = 1;
          break;
        case 'm':
          mflag = atoi(optarg);
          break;
        case 'i':
          input = optarg;
          break;
        case 'o':
          output = optarg;
          break;
        case '?':
          if (optopt=='m' || optopt=='i' || optopt=='o')
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

   if ((input == NULL) && (optind == argc - 1)) {
      input = argv[optind];
   }
   else if (optind < argc) {
      fprintf(stderr, "too many options\n");
      return 1;
   }

   FILE *inputf;
   FILE *outputf;

   if (input != NULL) {
      inputf = fopen(input, "r");
      if (inputf == NULL) {
         fprintf(stderr, "cannot open file %s\n", input);
         return 1;
      }
   }
   else {
      inputf = stdin;
   }

   if (output != NULL) {
      outputf = fopen(output, "w");
      if (outputf == NULL) {
         fprintf(stderr, "cannot write to file %s\n", output);
         return 1;
      }
   }
   else {
      outputf = stdout;
   }

   int exitcode = starcode(inputf, outputf, mflag, cflag);
   
   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;
}
#endif
