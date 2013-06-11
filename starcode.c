#include "starcode.h"

// Max length of a barcode.
#define MAXBRCDLEN 1024

int
cmp_nodes
(
   const void *a,
   const void *b
)
// Decreasing comparison of nodes by their counter.
{
   int A = (*(trienode **)a)->counter;
   int B = (*(trienode **)b)->counter;
   return (A < B) - (A > B);
}

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   // control //
   int compact_output
)
{

   int i;

   // The barcodes need to be counted first. Since we have
   // implemented a trie, we can use it for counting and indexing.
   // A positive side effect is that the data structure is also
   // compacted because the common prefixes between the barcodes
   // are counted only once.
   trienode *index = new_trie();

   size_t size = 256;
   char *barcode = malloc(size * sizeof(char));
   
   // Index lines of input file in a trie.
   int n_nodes = 0;
   int maxsize = 1024;
   trienode **nodes = malloc(maxsize * sizeof(trienode *));

   // Read the data from input file. We assume that it contains
   // one barcode per line and nothing else. 
   ssize_t read;
   while ((read = getline(&barcode, &size, inputf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';

      // Add barcode to index for counting.
      trienode *node = insert(index, barcode, 1);

      if (node->counter == 1) {
         // Reallocate 'nodes' if needed.
         if (n_nodes >= maxsize) {
            maxsize *= 2;
            trienode **ptr = realloc(nodes, maxsize * sizeof(trienode *));
            if (ptr != NULL) nodes = ptr;
            else {
               fprintf(stderr, "memory error\n");
               return 1;
            }
         }
         nodes[n_nodes] = node;
         n_nodes++;
      }
   }

   free(barcode);

   // Sort nodes in decreasing order by their counter. The first
   // node corresponds to the barcode sequence with highest
   // frequency.
   qsort(nodes, n_nodes, sizeof(trienode *), cmp_nodes);

   char buffer[MAXBRCDLEN];

   // Every barcode cluster has a prototype, which is the true
   // barcode sequences, the other are mutated or mis-sequenced
   // versions of it.
   trienode **prototypes = malloc(n_nodes * sizeof(trienode *));
   // Create and empty trie and use it to query the barcode
   // sequences one by one from most abundant to least abundant.
   // When no hit is found, the barcode is added to the trie,
   // otherwise, if there is a single match, that match is
   // marked as the prototype barcode of the query. The query
   // is allowed 3 mismatches. Note that not every barcode is
   // compared to every barcode; barcode are only queried
   // against prototypes. This saves a lot of computation time
   // and greatly simplifies the clustering algorithm.
   hitlist *hits = new_hitlist();
   trienode *trie = new_trie();

   for (i = 0 ; i < n_nodes ; i++) {
      char *barcode = seq(nodes[i], buffer, MAXBRCDLEN);
      // Search the index, with 3 allowed mismatches.
      // XXX could make this an argument passed to `starcode`.
      // So far, 3 seemed to be a good compromise, but it may
      // depend on barcode length, and sequencing depth.
      search(trie, barcode, 3, hits);
      if (hits->n_hits == 0) {
         // No hit. Add the barcode to the trie, and mark it
         // as a prototype.
         insert(trie, barcode, nodes[i]->counter);
         prototypes[i] = nodes[i];
      }
      // Exactly one hit. Mark the hit as the prototype of
      // the query.
      else if (hits->n_hits == 1) prototypes[i] = hits->node[0];
      // More than one hit. Do nothing.
      else prototypes[i] = NULL;
      clear_hitlist(hits);
   }

   if (compact_output) {
      // In compact output, only print the barcode and the count
      // of all barcodes of the culster (i.e. all the barcodes with
      // the same prototype).
      int n_clusters = 0;
      for (i = 0 ; i < n_nodes ; i++) {
         if (prototypes[i] == NULL) continue;
         if (nodes[i] == prototypes[i]) nodes[n_clusters++] = nodes[i];
         // Add to prototype count the counts of all the barcodes
         // in the cluster.
         else add_to_count(prototypes[i], nodes[i]->counter);
      }
      // Sort prototype barcodes by abundance and print.
      qsort(nodes, n_clusters, sizeof(trienode *), cmp_nodes);
      for (i = 0 ; i < n_clusters ; i++) {
         fprintf(outputf, "%s\t%d\n", seq(nodes[i], buffer, MAXBRCDLEN),
               nodes[i]->counter);
      }
   }
   else {
      // In non compact output, print every barcode, its prototype
      // and its count.
      char *buffer_a = buffer;
      char buffer_b[MAXBRCDLEN];
      fprintf(outputf, "barcode\tprototype\tcount\n");
      // Unlike for compact output there is no need to sort again
      // because the counts have not changed.
      for (i = 0 ; i < n_nodes ; i++) {
         if (prototypes[i] == NULL) continue;
         fprintf(outputf, "%s\t%s\t%d\n",
               seq(nodes[i], buffer_a, MAXBRCDLEN),
               seq(prototypes[i], buffer_b, MAXBRCDLEN),
               nodes[i]->counter);
      }
   }

   destroy_hitlist(hits);
   destroy_trie(index);
   destroy_trie(trie);
   free(nodes);
   free(prototypes);

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
   char *input = NULL;
   char *output = NULL;
   int c;

   while ((c = getopt (argc, argv, "cio:")) != -1)
      switch (c) {
        case 'c':
          cflag = 1;
          break;
        case 'i':
          input = optarg;
          break;
        case 'o':
          output = optarg;
          break;
        case '?':
          if (optopt == 'o')
            fprintf(stderr, "option -%o requires an argument\n", optopt);
          if (optopt == 'i')
            fprintf(stderr, "option -%i requires an argument\n", optopt);
          else if (isprint(optopt))
            fprintf(stderr, "unknown option `-%c'.\n", optopt);
          else
            fprintf(stderr, "unknown option `\\x%x'.\n", optopt);
          return 1;
        default:
          abort ();
      }

   if (optind == argc - 1) {
      input = argv[optind];
   }
   else {
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

   int exitcode = starcode(inputf, outputf, cflag);
   
   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;
}
#endif
