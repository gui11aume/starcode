#include "starcode.h"

#define str(a) *(char **)(a)

useq_t *new_useq(int, char*);
void destroy_useq (void*);
void addchild(useq_t*, useq_t*);
void print_children(useq_t*, useq_t*);
int abcd(const void *a, const void *b) { return strcmp(str(a), str(b)); }
int ucmp(const useq_t*, const useq_t*);
int bycount(const void*, const void*);


FILE *OUTPUT = NULL;

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   const int maxmismatch,
   const int verbose
)
{

   OUTPUT = outputf;

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

      // Link matching pairs.
      for (int j = 0 ; j < hits->idx ; j++) {
         useq_t *match = (useq_t *)hits->nodes[j]->data;
         useq_t *u1 = query;
         useq_t *u2 = match;
         if (ucmp(query, match) < 0) {
            u2 = query;
            u1 = match;
         }
         addchild(u1, u2);
      }

      // Insert the new sequence in the trie.
      node_t *node = insert_string(trie, query->seq);
      if (node == NULL) exit(EXIT_FAILURE);
      node->data = all_useq[i];
      if (verbose && (i % 100 == 99)) {
         fprintf(stderr, "starcode: %d/%d\r", i+1, utotal);
      }
   }

   free(hits);

   qsort(all_useq, utotal, sizeof(useq_t *), bycount);
   for (int i = 0 ; i < utotal ; i++) {
      print_children(all_useq[i], all_useq[i]);
   }         

   free(all_useq);
   destroy_nodes_downstream_of(trie, destroy_useq);

   if (verbose) fprintf(stderr, "\n");

   OUTPUT = NULL;

   return 0;

}


void
print_children
(
   useq_t *uref,
   useq_t *child
)
{

   // Check if the sequence has been processed.
   if (child->count == 0 || uref->count == 0) return;

   fprintf(OUTPUT, "%s:%d\t%s:%d\n",
         uref->seq, uref->count, child->seq, child->count);
   
   // Mark the child as processed.
   child->count = 0;

   if (child->children == NULL) return;
   for (int i = 0 ; i < child->children->idx ; i++) {
      print_children(uref, child->children->u[i]);
   }

}


void
addchild
(
   useq_t *parent,
   useq_t *child
)
{
   if (parent->children == NULL) {
      ustack_t *children = malloc(2*sizeof(int) + 16*sizeof(useq_t *));
      if (children == NULL) exit(EXIT_FAILURE);
      parent->children = children;
      children->lim = 16;
      children->idx = 0;
   }

   ustack_t *children = parent->children;

   if (children->idx >= children->lim) {
      int newlim = 2*children->lim;
      size_t newsize = 2*sizeof(int) + newlim*sizeof(useq_t *);
      ustack_t *ptr = realloc(children, newsize);
      if (ptr == NULL) exit(EXIT_FAILURE);
      parent->children = children = ptr;
      children->lim = newlim;
   }

   children->u[children->idx++] = child;

}


int
bycount
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = *(useq_t **)a;
   useq_t *u2 = *(useq_t **)b;
   return ucmp(u2, u1);
}


int
ucmp
(
   const useq_t *u1,
   const useq_t *u2
)
{
   if (u1->count > u2->count) return 1;
   if (u1->count == u2->count) return strcmp(u2->seq, u1->seq);
   return -1;
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
   new->children = NULL;
   return new;
}


void
destroy_useq
(
   void *u
)
{
   useq_t *useq = (useq_t *)u;
   if (useq->children != NULL) free(useq->children);
   free(useq->seq);
   free(useq);
}
