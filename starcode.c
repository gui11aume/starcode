#include "starcode.h"

#define str(a) *(char **)(a)

useq_t *new_useq(int, char*);
useq_t **get_useq(char**, int, int*);
char **read_file(FILE*, int*);
void destroy_useq (void*);
void print_and_destroy(void*);
void addchild(useq_t*, useq_t*);
void print_children(useq_t*, useq_t*);
void transfer_counts(useq_t*, useq_t*);
void unpad_useq (useq_t**, int);
int pad_useq(useq_t**, int);
int abcd(const void *a, const void *b) { return strcmp(str(a), str(b)); }
int ucmp(const useq_t*, const useq_t*);
int ualpha(const void*, const void*);
int bycount(const void*, const void*);


FILE *OUTPUT = NULL;
useq_t **ALL_USEQ = NULL;
trie_t *TRIE = NULL;
int TAU = 3;
int HEIGHT;

void *
build_trie
(
   void *arg_as_void
)
{
   // Cast arguments.
   int *arg = (int *) arg_as_void;

   trie_t *trie = new_trie(TAU, HEIGHT);
   nstack_t *hits = new_nstack();
   nstash_t *stash = new_nstash_for_trie(trie);
   if (trie == NULL || hits == NULL || stash == NULL) {
      fprintf(stderr, "error %d\n", check_trie_error_and_reset());
      exit(EXIT_FAILURE);
   }

   int start = 0;
   for (int i = arg[0] ; i < arg[1] ; i++) {
      char prefix[MAXBRCDLEN];
      useq_t *query = ALL_USEQ[i];
      int trail = 0;

      if (i < arg[1]-1) {
         useq_t *next_query = ALL_USEQ[i+1];
         while (query->seq[trail] == next_query->seq[trail]) {
            prefix[trail] = query->seq[trail];
            trail++;
         }
         prefix[trail] = '\0';
      }

      // Insert the prefix in the trie.
      node_t *node = insert_string(trie, prefix);
      if (node == NULL) {
         fprintf(stderr, "error %d\n", check_trie_error_and_reset());
         exit(EXIT_FAILURE);
      }

      // Clear hits.
      hits->pos = 0;
      int err = search(trie, query->seq, TAU,
            &hits, stash, start, trail, NULL, NULL);
      if (err) {
         fprintf(stderr, "error %d\n", err);
         exit(EXIT_FAILURE);
      }

      // Insert the rest of the new sequence in the trie.
      node = insert_string(trie, query->seq);
      if (node == NULL) {
         fprintf(stderr, "error %d\n", check_trie_error_and_reset());
         exit(EXIT_FAILURE);
      }
      node->data = ALL_USEQ[i];

      if (hits->err) {
         hits->err = 0;
         fprintf(stderr, "search error: %s\n", query->seq);
         continue;
      }

      // Link matching pairs.
      for (int j = 0 ; j < hits->pos ; j++) {
         useq_t *match = (useq_t *)hits->nodes[j]->data;
         // No relationship if count is equal.
         if (query->count == match->count) continue;
         useq_t *u1;
         useq_t *u2;
         if (query->count > match->count) {
            u1 = query;
            u2 = match;
         }
         else {
            u2 = query;
            u1 = match;
         }
         addchild(u1, u2);
      }

      if ((i & 4095) == 2048) {
         fprintf(stderr, "starcode: %d\n", i);
      }

      start = trail;

   }

   free(hits);
   destroy_nstash(stash);
   return (void *) trie;

}



void *
query_trie
(
   void *arg_as_void
)
{
   // Cast arguments.
   int *arg = (int *) arg_as_void;
   nstack_t *hits = new_nstack();
   nstash_t *stash = new_nstash_for_trie(TRIE);
   char *cache = new_external_cache(TRIE->size);
   if (hits == NULL || cache == NULL) {
      fprintf(stderr, "error %d\n", check_trie_error_and_reset());
      exit(EXIT_FAILURE);
   }

   int start = 0;
   for (int i = arg[0] ; i < arg[1] ; i++) {
      useq_t *query = ALL_USEQ[i];
      int trail = 0;

      if (i < arg[1]-1) {
         useq_t *next_query = ALL_USEQ[i+1];
         while (query->seq[trail] == next_query->seq[trail]) {
            trail++;
         }
      }

      // Clear hits.
      hits->pos = 0;
      int err = search(TRIE, query->seq, TAU,
            &hits, stash, start, trail, map_to_external, cache);

      if (err) {
         fprintf(stderr, "error %d\n", err);
         exit(EXIT_FAILURE);
      }
      if (hits->err) {
         hits->err = 0;
         fprintf(stderr, "search error: %s\n", query->seq);
         continue;
      }

      // Link matching pairs.
      for (int j = 0 ; j < hits->pos ; j++) {
         useq_t *match = (useq_t *)hits->nodes[j]->data;
         // No relationship if count is equal.
         if (query->count == match->count) continue;
         useq_t *u1;
         useq_t *u2;
         if (query->count > match->count) {
            u1 = query;
            u2 = match;
         }
         else {
            u2 = query;
            u1 = match;
         }
         addchild(u1, u2);
      }

      if ((i & 4095) == 2048) {
         fprintf(stderr, "starcode: %d\n", i);
      }

      start = trail;

   }

   destroy_nstash(stash);
   free(cache);
   return NULL;

}
 
int
tquery
(
   FILE *indexf,
   FILE *queryf,
   FILE *outputf,
   const int tau,
   const int verbose
)
{

   OUTPUT = outputf;

   if (verbose) fprintf(stderr, "reading input files...");
   int ni_seq;
   int nq_seq;
   char **all_i_seq = read_file(indexf, &ni_seq);
   char **all_q_seq = read_file(queryf, &nq_seq);
   if (verbose) fprintf(stderr, " done\n");

   if (verbose) fprintf(stderr, "preprocessing...");
   int ni_u;
   int nq_u;
   useq_t **i_useq = get_useq(all_i_seq, ni_seq, &ni_u);
   useq_t **q_useq = get_useq(all_q_seq, nq_seq, &nq_u);
   free(all_i_seq); all_i_seq = NULL;
   free(all_q_seq); all_q_seq = NULL;

   // Bundle all pointers in a single array.
   useq_t **ptr = realloc(i_useq, (ni_u + nq_u) * sizeof(useq_t *));
   if (ptr == NULL) exit(EXIT_FAILURE);
   memcpy(ptr + ni_u, q_useq, nq_u * sizeof(useq_t *));
   free(q_useq);
   i_useq = ptr;
   q_useq = ptr + ni_u;

   // Get a joint maximum length.
   int height = pad_useq(i_useq, ni_u + nq_u);
   qsort(q_useq, nq_u, sizeof(useq_t *), ualpha);

   trie_t *trie = new_trie(tau, height);
   if (trie == NULL) exit(EXIT_FAILURE);
   for (int i = 0 ; i < ni_u ; i++) {
      useq_t *u = i_useq[i];
      // Set the count to 0 ('count' is used at query time).
      u->count = 0;
      node_t *node = insert_string(trie, u->seq);
      if (node == NULL) exit(EXIT_FAILURE);
      node->data = u;
   }
   if (verbose) fprintf(stderr, " done\n");

   // Start the query.
   nstack_t *hits = new_nstack();
   nstash_t *stash = new_nstash_for_trie(trie);
   char *cache = new_external_cache(trie->size);
   if (hits == NULL || stash == NULL || cache == NULL) {
      fprintf(stderr, "error %d\n", check_trie_error_and_reset());
      exit(EXIT_FAILURE);
   }

   int start = 0;
   for (int i = 0 ; i < nq_u ; i++) {
      useq_t *query = q_useq[i];
      int trail = 0;
      if (i < nq_u - 1) {
         useq_t *next_query = q_useq[i+1];
         for ( ; query->seq[trail] == next_query->seq[trail] ; trail++);
      }
      // Clear hits.
      hits->pos = 0;
      search(trie, query->seq, tau,
            &hits, stash, start, trail, map_to_external, cache);
      if (hits->err) {
         hits->err = 0;
         fprintf(stderr, "search error: %s\n", query->seq);
         continue;
      }

      if (hits->pos == 1) {
         useq_t *match = (useq_t *)hits->nodes[0]->data;
         // Transfer counts to match.
         match->count += query->count;
      }
      if (verbose && ((i & 255) == 128)) {
         fprintf(stderr, "tquery: %d/%d\r", i+1, nq_u);
      }
      start = trail;
   }
   if (verbose) fprintf(stderr, "tquery: %d/%d\n", nq_u, nq_u);

   unpad_useq(i_useq, ni_u);

   destroy_nstash(stash);
   free(cache);
   destroy_trie(trie, print_and_destroy);
   for (int i = 0 ; i < nq_u ; i++) destroy_useq(q_useq[i]);

   free(i_useq);
   free(hits);

   OUTPUT = NULL;

   return 0;

}


int
starcode
(
   FILE *inputf,
   FILE *outputf,
   const int tau,
   const int n_threads,
   const int fmt,
   const int verbose
)
{

   OUTPUT = outputf;

   if (verbose) fprintf(stderr, "reading input file...");
   int total;
   char **all_seq = read_file(inputf, &total);
   if (verbose) fprintf(stderr, " done\n");

   if (verbose) fprintf(stderr, "preprocessing...");
   int utotal;
   useq_t **all_useq = get_useq(all_seq, total, &utotal);
   free(all_seq);

   HEIGHT = pad_useq(all_useq, utotal);
   qsort(all_useq, utotal, sizeof(useq_t *), ualpha);
   if (verbose) fprintf(stderr, " done\n");

   ALL_USEQ = all_useq;
   TAU = tau;

   pthread_t tid[2];
   int bkpts[3]; 
   for (int i = 0 ; i < n_threads + 1 ; i++) {
      bkpts[i] = (i * utotal) / n_threads;
   }

   for (int i = 0 ; i < 2 ; i++) {
      int err = pthread_create(tid+i, NULL, build_trie, bkpts+i);
      if (err) {
         fprintf(stderr, "Fucking hell (%d)!!!!\n", err);
         return err;
      }   
   }

   void *tries[2];
   for (int i = 0 ; i < 2 ; i++) pthread_join(tid[i], tries+i);

   // Now match the queries in the constructed tries.
   // We won't need the second trie.
   TRIE = tries[0];

   bkpts[0] = bkpts[1];
   bkpts[1] += 0.3 * (bkpts[2] - bkpts[1]);
   for (int i = 0 ; i < 2 ; i++) {
      int err = pthread_create(tid+i, NULL, query_trie, bkpts+i);
      if (err) {
         fprintf(stderr, "Fucking hell (%d)!!!!\n", err);
         return err;
      }   
   }

   for (int i = 0 ; i < 2 ; i++) pthread_join(tid[i], NULL);
   
   if (verbose) {
      fprintf(stderr, "starcode: %d/%d\n", utotal, utotal);
   }

   unpad_useq(all_useq, utotal);

   if (fmt == 0) {
      for (int i = 0 ; i < utotal ; i++) {
         useq_t *parent = all_useq[i];
         if (parent->children == NULL) continue;
         for (int j = 0 ; j < parent->children->pos ; j++) {
            transfer_counts(parent, parent->children->u[j]);
         }
      }
      qsort(all_useq, utotal, sizeof(useq_t *), bycount);
      for (int i = 0 ; i < utotal ; i++) {
         useq_t *u = all_useq[i];
         // Do not show sequences with 0 count.
         if (u->count == 0) break;
         fprintf(OUTPUT, "%s\t%d\n", u->seq, u->count);
      }
   }

   else if (fmt == 1) {
      qsort(all_useq, utotal, sizeof(useq_t *), bycount);
      for (int i = 0 ; i < utotal ; i++) {
         print_children(all_useq[i], all_useq[i]);
      }
   }

   free(all_useq);
   destroy_trie(tries[0], NULL);
   destroy_trie(tries[1], NULL);
   // TODO: Destroy useq.

   OUTPUT = NULL;

   return 0;

}


char **
read_file
(
   FILE *inputf,
   int *nlines
)
{
   size_t size = 1024;
   char **all_seq = malloc(size * sizeof(char *));
   if (all_seq == NULL) exit(EXIT_FAILURE);

   *nlines = 0;
   ssize_t nread;
   size_t nchar = MAXBRCDLEN;
   char *seq = malloc(MAXBRCDLEN * sizeof(char));
   // Read sequences from input file and store in an array. Assume
   // that it contains one sequence per line and nothing else. 
   while ((nread = getline(&seq, &nchar, inputf)) != -1) {
      // Strip end of line character and copy.
      if (seq[nread-1] == '\n') seq[nread-1] = '\0';
      char *new = malloc(nread);
      if (new == NULL) exit(EXIT_FAILURE);
      strncpy(new, seq, nread);
      // Grow 'all_seq' if needed.
      if (*nlines >= size) {
         size *= 2;
         char **ptr = realloc(all_seq, size * sizeof(char *));
         if (ptr == NULL) exit(EXIT_FAILURE);
         all_seq = ptr;
      }
      all_seq[(*nlines)++] = new;
   }

   free(seq);
   return all_seq;

}


useq_t **
get_useq
(
   char ** all_seq,
   int     total,
   int  *  utotal
)
{
   // Sort sequences, count and compact them. Sorting
   // alphabetically is important for speed.
   qsort(all_seq, total, sizeof(char *), abcd);

   int size = 1024;
   useq_t **all_useq = malloc(size * sizeof(useq_t *));
   *utotal = 0;
   int ucount = 0;
   for (int i = 0 ; i < total-1 ; i++) {
      ucount++;
      if (strcmp(all_seq[i], all_seq[i+1]) == 0) free(all_seq[i]);
      else {
         all_useq[(*utotal)++] = new_useq(ucount, all_seq[i]);
         ucount = 0;
         // Grow 'all_useq' if needed.
         if (*utotal >= size) {
            size *= 2;
            useq_t **ptr = realloc(all_useq, size * sizeof(useq_t *));
            if (ptr == NULL) exit(EXIT_FAILURE);
            all_useq = ptr;
         }
      }
   }
   all_useq[(*utotal)++] = new_useq(ucount+1, all_seq[total-1]);

   return all_useq;

}


int
pad_useq
(
   useq_t ** all_useq,
   int       utotal
)
{
   // Compute maximum length.
   int maxlen = 0;
   for (int i = 0 ; i < utotal ; i++) {
      int len = strlen(all_useq[i]->seq);
      if (len > maxlen) maxlen = len;
   }

   char *spaces = malloc((maxlen + 1) * sizeof(char));
   for (int i = 0 ; i < maxlen ; i++) spaces[i] = ' ';
   spaces[maxlen] = '\0';

   // Pad all sequences with spaces.
   for (int i = 0 ; i < utotal ; i++) {
      int len = strlen(all_useq[i]->seq);
      if (len == maxlen) continue;
      char *padded = malloc((maxlen + 1) * sizeof(char));
      memcpy(padded, spaces, maxlen + 1);
      memcpy(padded+maxlen-len, all_useq[i]->seq, len);
      free(all_useq[i]->seq);
      all_useq[i]->seq = padded;
   }

   free(spaces);

   return maxlen;

}


void
unpad_useq
(
   useq_t ** all_useq,
   int       utotal
)
{
   int len = strlen(all_useq[0]->seq);
   for (int i = 0 ; i < utotal ; i++) {
      int pad = 0; 
      while (all_useq[i]->seq[pad] == ' ') pad++;
      char *unpadded = malloc((len - pad + 1) * sizeof(char));
      memcpy(unpadded, all_useq[i]->seq + pad, len - pad + 1);
      free(all_useq[i]->seq);
      all_useq[i]->seq = unpadded;
   }
}


void
transfer_counts
(
   useq_t *uref,
   useq_t *child
)
{

   // Check if the sequence has been processed.
   if (uref->count == 0 || child->count == 0) return;

   //fprintf(OUTPUT, "%s:%d\t%s:%d\n",
   //      child->seq, child->count, uref->seq, uref->count);
   uref->count += child->count;
   
   if (child->children != NULL) {
      for (int i = 0 ; i < child->children->pos ; i++) {
         transfer_counts(uref, child->children->u[i]);
      }
   }

   // Mark the child as processed.
   child->count = 0;

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
         child->seq, child->count, uref->seq, uref->count);
   
   if (child->children != NULL) {
      for (int i = 0 ; i < child->children->pos ; i++) {
         print_children(uref, child->children->u[i]);
      }
   }

   // Mark the child as processed.
   child->count = 0;

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
      children->pos = 0;
   }

   ustack_t *children = parent->children;

   if (children->pos >= children->lim) {
      int newlim = 2*children->lim;
      size_t newsize = 2*sizeof(int) + newlim*sizeof(useq_t *);
      ustack_t *ptr = realloc(children, newsize);
      if (ptr == NULL) exit(EXIT_FAILURE);
      parent->children = children = ptr;
      children->lim = newlim;
   }

   children->u[children->pos++] = child;

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


int
ualpha
(
   const void *a,
   const void *b
) 
{
   useq_t *u1 = *(useq_t **)a;
   useq_t *u2 = *(useq_t **)b;
   return strcmp(u2->seq, u1->seq);
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
print_and_destroy
(
   void *u
)
{
   useq_t *useq = (useq_t *)u;
   fprintf(OUTPUT, "%s\t%d\n", useq->seq, useq->count);
   destroy_useq(useq);
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
