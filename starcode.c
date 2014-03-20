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

   node_t *trie = new_trie(tau, height);
   if (trie == NULL) exit(EXIT_FAILURE);
   for (int i = 0 ; i < ni_u ; i++) {
      useq_t *u = i_useq[i];
      // Set the count to 0 ('count' is used at query time).
      u->count = 0;
      node_t *node = insert_string(trie, u->seq);
      if (node == NULL) exit(EXIT_FAILURE);
      if (node != trie) node->data = u;
   }
   if (verbose) fprintf(stderr, " done\n");

   // Start the query.
   narray_t *hits = new_narray();
   if (hits == NULL) exit(EXIT_FAILURE);

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
      search(trie, query->seq, tau, &hits, start, trail);
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

   int height = pad_useq(all_useq, utotal);
   qsort(all_useq, utotal, sizeof(useq_t *), ualpha);
   if (verbose) fprintf(stderr, " done\n");

   // Compute multithreading plan
   if (verbose) fprintf(stderr,"Computing multithreading plan...");
   mtplan_t * mtplan = prepare_mtplan(tau, height, utotal, all_useq);
   if (verbose) fprintf(stderr," done\n");

   // Assign threads to their jobs
   for (int c = 0; c < mtplan->numconts; c++) {
      mtcontext_t context = mtplan->context[c];

      // Start jobs
      for (int j = 0; j < context.numjobs; j++) {
         pthread_t thread;
         mtjob_t *job = context.jobs + j;

         if (pthread_create(&thread, NULL, starcode_thread, (void *) job) != 0) {
            fprintf(stderr, "error: job failed to start (context no. %d, job no. %d).\n",c,j);
            exit(EXIT_FAILURE);
         }
         context.jobs[j].thread = thread;
      }
      
      if (verbose) {
         fprintf(stderr, "mt context: %d/%d\r", c, mtplan->numconts);
      }

      // Join all threads
      for (int j = 0; j < context.numjobs; j++)
         pthread_join(context.jobs[j].thread, NULL);

   }

   if (verbose) {
      fprintf(stderr, "mt context: %d/%d\n", mtplan->numconts, mtplan->numconts);
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
   // Destroy tries malloc'ed at context 0 (build trie)
   for (int t = 0; t < mtplan->context[0].numjobs; t++)
      destroy_trie(mtplan->context[0].jobs[t].trie, destroy_useq);

   OUTPUT = NULL;

   return 0;

}



void *
starcode_thread
(
   void * args
)  
{
   // Unpack arguments.

   mtjob_t * job = (mtjob_t*) args;
   useq_t ** all_useq = job->all_useq;
   node_t  * trie = job->trie;
   int tau = job->tau;

   // Create local hits narray.
   narray_t *hits = new_narray();
   if (hits == NULL) {
      fprintf(stderr, "error %d\n", check_trie_error_and_reset());
      exit(EXIT_FAILURE);
   }

   int start = 0;
   for (int i = job->start ; i <= job->end ; i++) {
      char prefix[MAXBRCDLEN];
      useq_t *query = all_useq[i];
      int trail = 0;

      prefix[0] = '\0';
      if (i < job->end) {
         useq_t *next_query = all_useq[i+1];
         while (query->seq[trail] == next_query->seq[trail]) {
            prefix[trail] = query->seq[trail];
            trail++;
         }
         prefix[trail] = '\0';
      }

      // Insert the prefix in the trie.
      node_t *node;
      if (job->build) {
         node = insert_string(trie, prefix);
         if (node == NULL) {
            fprintf(stderr, "error %d\n", check_trie_error_and_reset());
            exit(EXIT_FAILURE);
         }
      }

      // Clear hits.
      hits->pos = 0;
      int err = search(trie, query->seq, tau, &hits, start, trail);
      if (err) {
         fprintf(stderr, "error %d\n", err);
         exit(EXIT_FAILURE);
      }

      // Insert the rest of the new sequence in the trie.
      if (job->build) {
         node = insert_string(trie, query->seq);
         if (node == NULL) {
            fprintf(stderr, "error %d\n", check_trie_error_and_reset());
            exit(EXIT_FAILURE);
         }
         if (node != trie) node->data = all_useq[i];
      }

      if (hits->err) {
         hits->err = 0;
         fprintf(stderr, "search error: %s\n", query->seq);
         continue;
      }

      // Link matching pairs. (No need for mutex when building)
      for (int j = 0 ; j < hits->pos ; j++) {
         // Do not mess up with root node.
         if (hits->nodes[j] == trie) continue;
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
         if (job->build) pthread_mutex_lock(job->mutex);
         addchild(u1, u2);
         if (job->build) pthread_mutex_unlock(job->mutex);
      }

      start = trail;
   }
   
   free(hits);

   return NULL;
}

mtplan_t *
prepare_mtplan
(
   int      tau,
   int      height,
   int      utotal,
   useq_t** useq
)
{
   // Initialize plan.
   mtplan_t *plan = (mtplan_t*) malloc(sizeof(mtplan_t));

   // Initialize mutex
   pthread_mutex_t *mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
   pthread_mutex_init(mutex,NULL);

   // Compute the first context (build tries).
   int nbases = strlen(BASES);
   int ntries = 0;
   mtjob_t *bjobs = (mtjob_t *) malloc(nbases * sizeof(mtjob_t));

   char b[2] = "X";
   for (int i = 0; i < nbases; i++) {
      b[0] = BASES[i];
      // Find prefixes using bisection.
      int start = bisection(0, utotal-1, b, useq, 1, BISECTION_START);
      if (BASES[i] != useq[start]->seq[0]) continue;
      int end = bisection(0, utotal-1, b, useq, 1, BISECTION_END);

      // Fill mtjob.
      bjobs[i].start    = start;
      bjobs[i].end      = end;
      bjobs[i].tau      = tau;
      bjobs[i].build    = 1;
      bjobs[i].all_useq = useq;
      bjobs[i].trie     = new_trie(tau,height);
      bjobs[i].mutex    = mutex;

      ntries++;
   }

   // Compute number of contexts.
   int ncont = 1;
   while (ntries/ncont != 2) ncont++;
   // Plus the build context.
   ncont += 1;
   
   // Initialize contexts.
   plan->context = (mtcontext_t *) malloc(ncont * sizeof(mtcontext_t));
   plan->numconts = ncont;

   // Save first context.
   plan->context[0].numjobs = ntries;
   plan->context[0].jobs = bjobs;

   // Compute query contexts.
   int njobs = ntries;
   for (int c = 1; c < ncont; c++) {
      if (c == ncont - 1) {
         // Fill the remaining query combinations.
         int combs = 0;
         for (int i = ntries - 1; i > 0; i--) combs += i;
         njobs = combs % ntries;
         if (njobs == 0) njobs = ntries;
      }
      plan->context[c].numjobs = njobs;
      plan->context[c].jobs = (mtjob_t *) malloc(njobs * sizeof(mtjob_t));
      for (int j = 0; j < njobs; j++) {
         mtjob_t * job = plan->context[c].jobs + j;
         // Fill mtjob.
         job->tau       = tau;
         job->mutex     = mutex;
         // Query seq.
         job->all_useq  = useq;
         job->start     = plan->context[0].jobs[j].start;
         job->end       = plan->context[0].jobs[j].end;
         // Trie to query (query tries shifted by c).
         job->trie      = plan->context[0].jobs[(j+c)%ntries].trie;
         // Do not build, just query.
         job->build     = 0;
      }
   }

   return plan;
}

int
bisection
(
   int       start,
   int       end,
   char    * query,
   useq_t ** useq,
   int       dist,
   int       direction
)
{
   int pos = (start + end)/2;
   int grad = strncmp(query, useq[pos]->seq,dist);
   if (end - start == 1) return (grad == 0 ? start : end);
   if (grad == 0) grad = direction;
   if (grad > 0) return bisection(start, pos, query, useq, dist, direction);
   else return bisection(pos , end, query, useq, dist, direction);
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
      // Line commented to avoid deadlocks
      //if (children == NULL) exit(EXIT_FAILURE);
      parent->children = children;
      children->lim = 16;
      children->pos = 0;
   }

   ustack_t *children = parent->children;

   if (children->pos >= children->lim) {
      int newlim = 2*children->lim;
      size_t newsize = 2*sizeof(int) + newlim*sizeof(useq_t *);
      ustack_t *ptr = realloc(children, newsize);
      // Line commented to avoid deadlocks
      //if (ptr == NULL) exit(EXIT_FAILURE); 
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
