#include "starcode.h"

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
   int compact_output
)
{

   int i;

   trienode *index = new_trie();

   size_t size = 256;
   char *barcode = malloc(size * sizeof(char));
   
   // Index lines of input file in a trie.
   int n_nodes = 0;
   int maxsize = 1024;
   trienode **nodes = malloc(maxsize * sizeof(trienode *));

   ssize_t read;
   while ((read = getline(&barcode, &size, inputf)) != -1) {
      // Strip end of line character.
      if (barcode[read-1] == '\n') barcode[read-1] = '\0';

      // Add barcode to index.
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

   // Sort nodes in decreasing order by their counter.
   qsort(nodes, n_nodes, sizeof(trienode *), cmp_nodes);

   char buffer[MAXBRCDLEN];

   hitlist *hits = new_hitlist();
   trienode *trie = new_trie();
   trienode **models = malloc(n_nodes * sizeof(trienode *));

   for (i = 0 ; i < n_nodes ; i++) {
      char *barcode = seq(nodes[i], buffer, MAXBRCDLEN);
      search(trie, barcode, 3, hits);
      if (hits->n_hits == 0) {
         insert(trie, barcode, nodes[i]->counter);
         models[i] = nodes[i];
      }
      else if (hits->n_hits == 1) models[i] = hits->node[0];
      else models[i] = NULL;
      clear_hitlist(hits);
   }

   if (compact_output) {
      int n_clusters = 0;
      for (i = 0 ; i < n_nodes ; i++) {
         if (models[i] == NULL) continue;
         if (nodes[i] == models[i]) nodes[n_clusters++] = nodes[i];
         else add_to_count(models[i], nodes[i]->counter);
      }
      qsort(nodes, n_clusters, sizeof(trienode *), cmp_nodes);
      for (i = 0 ; i < n_clusters ; i++) {
         fprintf(outputf, "%s\t%d\n", seq(nodes[i], buffer, MAXBRCDLEN),
               nodes[i]->counter);
      }
   }
   else {
      char *buffer_a = buffer;
      char buffer_b[MAXBRCDLEN];
      fprintf(outputf, "barcode\tmodel\tcount\n");
      for (i = 0 ; i < n_nodes ; i++) {
         if (models[i] == NULL) continue;
         fprintf(outputf, "%s\t%s\t%d\n",
               seq(nodes[i], buffer_a, MAXBRCDLEN),
               seq(models[i], buffer_b, MAXBRCDLEN),
               nodes[i]->counter);
      }
   }

   destroy_hitlist(hits);
   destroy_trie(index);
   destroy_trie(trie);
   free(nodes);
   free(models);

   return 0;

}

#ifndef _test_starcode
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
