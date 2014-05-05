#include "starcode.h"
#include "trie.h"

#define str(a) (char *)(a)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

useq_t *new_useq(int, char*);
gstack_t *seq2useq(gstack_t*, int);
gstack_t *read_file(FILE*);
void destroy_useq (useq_t*);
void addmatch(useq_t*, useq_t*, int, int);
void transfer_counts(useq_t*);
void unpad_useq (gstack_t*);
int pad_useq(gstack_t*);
int AtoZ(const void *a, const void *b) { return strcmp(str(a), str(b)); }
int maxtomin(const void*, const void*);
int mintomax(const void*, const void*);

FILE *OUTPUT = NULL;

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   const int tau,
   const int fmt,
   const int verbose,
   const int maxthreads,
         int ntries,
   const int mtstrategy
)
{
   // XXX Scheduling is much better balanced if there are
   // an odd number of tries. Make this clean.
   if (ntries % 2 == 0) ntries++;
   OUTPUT = outputf;
   
   if (verbose) fprintf(stderr, "reading input files\n");
   gstack_t *seqS = read_file(inputf);

   if (verbose) fprintf(stderr, "preprocessing\n");
   // Count unique sequences.
   gstack_t *useqS = seq2useq(seqS, maxthreads);
   free(seqS);
   int height = pad_useq(useqS);

   // Make multithreading plan.
   mtplan_t *mtplan = plan_mt(tau, height, ntries, useqS);

   // Count total number of jobs.
   int njobs = 0;
   for (int t = 0; t < mtplan->ntries; t++)
      njobs +=  mtplan->tries[t].njobs;

   // Thread Scheduler
   int done = 0;
   int jobsdone = (-mtplan->ntries > -maxthreads ? -mtplan->ntries : -maxthreads);
   int t = 0;
   mttrie_t *mttrie;


   while (done < mtplan->ntries) { 

      // Let's make a fair scheduler.
      t = (t + 1) % mtplan->ntries;

      mttrie = mtplan->tries + t;

      // Check whether the trie is free and there are available threads
      pthread_mutex_lock(mtplan->mutex);     
      if (mttrie->flag == TRIE_FREE && mtplan->threadcount < maxthreads) {

         // When trie is done, count and flag.
         if (mttrie->currentjob == mttrie->njobs) {
            mttrie->flag = TRIE_DONE;
            done++;
         }

         // If not yet done but free, assign next job.
         else {
            mtjob_t * job = mttrie->jobs + mttrie->currentjob++;
            mttrie->flag = TRIE_BUSY;
            mtplan->threadcount++;
            pthread_t thread;
            // Start job.
            if (pthread_create(&thread, NULL, starcode_thread, job) != 0) {
               fprintf(stderr,
                     "error: pthread_create (trie no. %d, job no. %d).\n",
                     t, mttrie->currentjob);
               exit(EXIT_FAILURE);
            }
            // Detach thread.
            pthread_detach(thread);

            if (verbose) {
               jobsdone++;
               fprintf(stderr, "Starcode progress: %.2f%% \r",
                     100*(float)(jobsdone)/njobs);
            }
         }
      }

      while (mtplan->threadcount == min(maxthreads, mtplan->ntries)) {
         pthread_cond_wait(mtplan->monitor, mtplan->mutex);
      }
      pthread_mutex_unlock(mtplan->mutex);
   }


   if (verbose) {
      fprintf(stderr, "Starcode progress: 100.00%%\n");
   }

   unpad_useq(useqS);

   // Sort from min to max.
   mergesort(useqS->items, useqS->nitems, mintomax, maxthreads);

   if (fmt == 0) {
      // Transfer counts to parents recursively.
      for (int i = 0 ; i < useqS->nitems ; i++) {
         useq_t *u = (useq_t *) useqS->items[i];
         transfer_counts(u);
      }

      // Sort from max to min.
      mergesort(useqS->items, useqS->nitems, maxtomin, maxthreads);
      for (int i = 0 ; i < useqS->nitems ; i++) {
         useq_t *u = (useq_t *) useqS->items[i];
         // Do not show sequences with 0 count.
         if (u->count == 0) break;
         fprintf(OUTPUT, "%s\t%d\n", u->seq, u->count);
      }
   }

   else if (fmt == 1) {
      fprintf(OUTPUT, "not implemented\n");
   }

   // WHY free? Will be freed in a usec.
   //free(all_useq);

   // Destroy tries malloc'ed at context 0 (build trie) 
   //   for (int t = 0; t < mtplan->ntries; t++)
   // destroy_trie(mtplan->tries[t].jobs[0].trie, destroy_useq);

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
   mtjob_t  * job   = (mtjob_t*) args;
   gstack_t * useqS = job->useqS;
   node_t   * trie  = job->trie;
   int        tau   = job->tau;

   // Create local hit stack.
   gstack_t **hits = new_tower(tau+1);
   if (hits == NULL) {
      fprintf(stderr, "error: cannot create hit stack (%d)\n",
            check_trie_error_and_reset());
      exit(EXIT_FAILURE);
   }

   int start = 0;
   for (int i = job->start ; i <= job->end ; i++) {
      char prefix[MAXBRCDLEN];
      useq_t *query = (useq_t *) useqS->items[i];

      prefix[0] = '\0';
      int trail = 0;
      if (i < job->end) {
         useq_t *next_query = (useq_t *) useqS->items[i+1];
         // The 'while' condition is guaranteed to be false
         // before the end of the 'char' arrays because all
         // the queries have the same length and are different.
         while (query->seq[trail] == next_query->seq[trail]) {
            prefix[trail] = query->seq[trail];
            trail++;
         }
         prefix[trail] = '\0';
      }

      // Insert the prefix in the trie. This is not a tail
      // node, so the 'data' pointer is left unset.
      node_t *node;
      if (job->build) {
         node = insert_string(trie, prefix);
         if (node == NULL) {
            fprintf(stderr, "error: cannot build trie (%d)\n",
                  check_trie_error_and_reset());
            exit(EXIT_FAILURE);
         }
      }

      // Clear hit stack.
      for (int j = 0 ; hits[j] != TOWER_TOP ; j++) hits[j]->nitems = 0;
      int err = search(trie, query->seq, tau, hits, start, trail);
      if (err) {
         fprintf(stderr, "error: cannot complete query (%d)\n", err);
         exit(EXIT_FAILURE);
      }

      // Insert the rest of the new sequence in the trie.
      if (job->build) {
         node = insert_string(trie, query->seq);
         if (node == NULL) {
            fprintf(stderr, "error: cannot build trie (%d)\n",
                  check_trie_error_and_reset());
            exit(EXIT_FAILURE);
         }
         // Set the 'data' pointer of the inserted tail node.
         // NB: 'node' cannot be the root of the trie because
         // the empty string is never inserted (see the warning
         // in the code of 'insert_string()'.
         node->data = query;
      }

      for (int j = 0 ; hits[j] != TOWER_TOP ; j++) {
      if (hits[j]->nitems > hits[j]->nslots) {
         fprintf(stderr, "warning: incomplete search results (%s)\n",
               query->seq);
         break;
      }
      }

      // Link matching pairs.
      for (int dist = 0 ; dist < tau+1 ; dist++) {
         for (int j = 0 ; j < hits[dist]->nitems ; j++) {
            node_t *match_node = (node_t *) hits[dist]->items[j];
            useq_t *match = (useq_t *) match_node->data;
            // No link if counts are on the same order of magnitude.
            int mincount = min(query->count, match->count);
            int maxcount = max(query->count, match->count);
            if ((maxcount < PARENT_TO_CHILD_FACTOR * mincount)) continue;

            // Add match to the sequence with least count.
            if (query->count > match->count) {
               pthread_mutex_lock(job->mutex);
               addmatch(query, match, dist, tau);
               pthread_mutex_unlock(job->mutex);
            }
            else {
               pthread_mutex_lock(job->mutex);
               addmatch(match, query, dist, tau);
               pthread_mutex_unlock(job->mutex);
            }
         }
      }
      start = trail;
   }
   
   destroy_tower(hits);

   // Flag trie, update thread count.
   pthread_mutex_lock(job->mutex);
   *(job->threadcount) -= 1;
   *(job->trieflag) = TRIE_FREE;
   pthread_mutex_unlock(job->mutex);

   // Signal scheduler
   pthread_cond_signal(job->monitor);

   return NULL;

}


mtplan_t *
plan_mt
(
    int       tau,
    int       height,
    int       ntries,
    gstack_t *useqS
)
// Jobs are either "build" or "query" type. A "query" job
// can only be done after a "build" job has built the trie.
{
   // Initialize plan.
   mtplan_t *mtplan = malloc(sizeof(mtplan_t));
   if (mtplan == NULL) exit(EXIT_FAILURE);

   // Initialize mutex.
   pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
   pthread_cond_t *monitor = malloc(sizeof(pthread_cond_t));
   if (mutex == NULL || monitor == NULL) exit(EXIT_FAILURE);
   pthread_mutex_init(mutex,NULL);
   pthread_cond_init(monitor,NULL);

   // Initialize 'mttries'.
   mttrie_t *mttries = malloc(ntries * sizeof(mttrie_t));
   if (mttries == NULL) exit(EXIT_FAILURE);

   // Boundaries of the query blocks.
   int Q = useqS->nitems / ntries;
   int R = useqS->nitems % ntries;
   int *bounds = malloc((ntries+1) * sizeof(int));
   for (int i = 0 ; i < ntries+1 ; i++) bounds[i] = Q*i + min(i, R);

   // Create jobs for the tries.
   for (int i = 0 ; i < ntries; i++) {
      // Remember that 'ntries' is odd.
      int njobs = (ntries+1)/2;
      node_t *local_trie = new_trie(tau, height);
      mtjob_t *jobs = malloc(njobs * sizeof(mtjob_t));
      if (local_trie == NULL || jobs == NULL) exit(EXIT_FAILURE);

      mttries[i].flag       = TRIE_FREE;
      mttries[i].currentjob = 0;
      mttries[i].njobs      = njobs;
      mttries[i].jobs       = jobs;

      for (int j = 0 ; j < njobs ; j++) {
         // Shift boundaries in a way that every trie is built
         // exactly once and that no redundant jobs are allocated.
         int idx = (i+j) % ntries;
         int only_if_first_job = j == 0;
         // Specifications of j-th job of the local trie.
         jobs[j].start       = bounds[idx];
         jobs[j].end         = bounds[idx+1]-1;
         jobs[j].tau         = tau;
         jobs[j].build       = only_if_first_job;
         jobs[j].useqS       = useqS;
         jobs[j].trie        = local_trie;
         jobs[j].mutex       = mutex;
         jobs[j].monitor     = monitor;
         jobs[j].trieflag    = &(mttries[i].flag);
         jobs[j].threadcount = &(mtplan->threadcount);
      }
   }

   free(bounds);

   mtplan->threadcount = 0;
   mtplan->ntries = ntries;
   mtplan->mutex = mutex;
   mtplan->monitor = monitor;
   mtplan->tries = mttries;

   return mtplan;
}


void *
_mergesort
(
 void * args
)
{
   sortargs_t * sortargs = (sortargs_t *) args;
   if (sortargs->size > 1) {
      // Next level params.
      sortargs_t arg1 = *sortargs, arg2 = *sortargs;
      arg1.size /= 2;
      arg2.size = arg1.size + arg2.size % 2;
      arg2.buf0 += arg1.size;
      arg2.buf1 += arg1.size;
      arg1.b = arg2.b = (arg1.b + 1) % 2;

      // Either run threads or DIY.
      if (arg1.thread) {
         // Decrease one level.
         arg1.thread = arg2.thread = arg1.thread - 1;
         // Create threads.
         pthread_t thread1, thread2;
         pthread_create(&thread1, NULL, _mergesort, (void *) &arg1);
         pthread_create(&thread2, NULL, _mergesort, (void *) &arg2);
         // Wait for threads.
         pthread_join(thread1, NULL);
         pthread_join(thread2, NULL);
      }
      else {
         _mergesort((void *) &arg1);
         _mergesort((void *) &arg2);
      }

      // Separate data and buffer (b specifies which is buffer).
      void ** l = (sortargs->b ? arg1.buf0 : arg1.buf1);
      void ** r = (sortargs->b ? arg2.buf0 : arg2.buf1);
      void ** buf = (sortargs->b ? arg1.buf1 : arg1.buf0);

      // Accumulate repeats
      sortargs->repeats = arg1.repeats + arg2.repeats;

      // Merge sets
      int i = 0, j = 0, nulli = 0, nullj = 0, cmp = 0, repeats = 0;
      for (int k = 0, idx = 0, n = 0; k < sortargs->size; k++) {
         if (j == arg2.size) {
            // Insert pending nulls, if any.
            for (n = 0; n < nulli; n++)
               buf[idx++] = NULL;
            nulli = 0;
            buf[idx++] = l[i++];
         }
         else if (i == arg1.size) {
            // Insert pending nulls, if any.
            for (n = 0; n < nullj; n++)
               buf[idx++] = NULL;
            nullj = 0;
            buf[idx++] = r[j++];
         }
         else if (l[i] == NULL) {
            nulli++;
            i++;
         }
         else if (r[j] == NULL) {
            nullj++;
            j++;
         }
         else if ((cmp = sortargs->compar(l[i],r[j])) == 0) {
            j++;
            repeats++;
            // Insert sum of repeats as NULL.
            for (n = 0; n <= nulli + nullj; n++) {
               buf[idx++] = NULL;
            }
            nulli = nullj = 0;
         } 
         else if (cmp < 0) {
            // Insert repeats as NULL.
            for (n = 0; n < nulli; n++)
               buf[idx++] = NULL;
            nulli = 0;

            buf[idx++] = l[i++];
         }
         else {
            // Insert repeats as NULL.
            for (n = 0; n < nullj; n++)
               buf[idx++] = NULL;
            nullj = 0;

            buf[idx++] = r[j++];
         }
      }
      sortargs->repeats += repeats;
   }
   
   return NULL;

}

int
mergesort
(
 void **data,
 int numels,
 int (*compar)(const void*, const void*),
 int maxthreads
)
{
   // Copy to buffer.
   void **buffer = malloc(numels * sizeof(void *));
   memcpy(buffer, data, numels * sizeof(void *));

   // Prepare args struct.
   sortargs_t args;
   args.buf0   = data;
   args.buf1   = buffer;
   args.size   = numels;
   args.b      = 0; // Important so that sorted elements end in data (not in buffer).
   args.thread = 0;
   args.repeats = 0;
   args.compar = compar;
   while ((maxthreads >> (args.thread + 1)) > 0) args.thread++;

   _mergesort(&args);

   free(buffer);
   
   return numels - args.repeats;
}


gstack_t *
read_file
(
   FILE *inputf
)
{
   gstack_t *seqS = new_gstack();
   ssize_t nread;
   size_t nchar = MAXBRCDLEN;
   char *seq = malloc(MAXBRCDLEN * sizeof(char));
   // Read sequences from input file and store in an array. Assume
   // that it contains one sequence per line and nothing else. 
   while ((nread = getline(&seq, &nchar, inputf)) != -1) {
      // Strip end of line character.
      if (seq[nread-1] == '\n') seq[nread-1] = '\0';
      // Copy and push to stack.
      char *new = malloc(nread * sizeof(char));
      if (new == NULL) exit(EXIT_FAILURE);
      strncpy(new, seq, nread);
      push(new, &seqS);
   }
   free(seq);
   return seqS;
}



gstack_t *
seq2useq
(
   gstack_t *seqS,
   int     maxthreads
)
{
   // Sort sequences, count and compact them.
   mergesort(seqS->items, seqS->nitems, AtoZ, maxthreads);
   gstack_t *useqS = new_gstack();
   
   int ucount = 0;
   for (int i = 0 ; i < seqS->nitems; i++) {
      ucount++;
      // Repeats have been set to 'NULL' during sort.
      if (seqS->items[i] != NULL) {
         push(new_useq(ucount, (char *) seqS->items[i]), &useqS);
         ucount = 0;
      }
   }
   return useqS;
}


int
pad_useq
(
   gstack_t *useqS
)
{
   // Compute maximum length.
   int maxlen = 0;
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      if (len > maxlen) maxlen = len;
   }

   char *spaces = malloc((maxlen + 1) * sizeof(char));
   for (int i = 0 ; i < maxlen ; i++) spaces[i] = ' ';
   spaces[maxlen] = '\0';

   // Pad all sequences with spaces.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      if (len == maxlen) continue;
      // Create a new sequence with padding characters.
      char *padded = malloc((maxlen + 1) * sizeof(char));
      memcpy(padded, spaces, maxlen + 1);
      memcpy(padded+maxlen-len, u->seq, len);
      free(u->seq);
      u->seq = padded;
   }
   free(spaces);
   return maxlen;
}


void
unpad_useq
(
   gstack_t *useqS
)
{
   // Take the length of the first sequence (assume all
   // sequences have the same length).
   int len = strlen(((useq_t *) useqS->items[0])->seq);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = (useq_t *) useqS->items[i];
      int pad = 0;
      while (u->seq[pad] == ' ') pad++;
      // Create a new sequence without paddings characters.
      char *unpadded = malloc((len - pad + 1) * sizeof(char));
      if (unpadded == NULL) exit(EXIT_FAILURE);
      memcpy(unpadded, u->seq + pad, len - pad + 1);
      free(u->seq);
      u->seq = unpadded;
   }
   return;
}


void
transfer_counts
(
   useq_t *useq
)
{
   // Break on parent nodes and empty nodes.
   if (useq->count == 0 || useq->matches == NULL) return;

   // Get the lowest match stratum.
   gstack_t *matches;
   for (int i = 0 ; (matches = useq->matches[i]) != TOWER_TOP ; i++) {
      if (matches->nitems > 0) break;
   }

   // Distribute counts evenly among parents.
   int Q = useq->count / matches->nitems;
   int R = useq->count % matches->nitems;
   // Depending on the order in which matches were made
   // (which is more or less random), the transferred
   // counts can differ by 1. For this reason, the output
   // can be slightly different for different tries numbers.
   for (int i = 0; i < matches->nitems; i++) {
      match_t *match = (match_t *) matches->items[i];
      match->useq->count += Q + (i < R);
   }
   // Transfer done.
   useq->count = 0;

   // Continue propagation.
   for (int i = 0 ; i < matches->nitems ; i++) {
      match_t *match = (match_t *) matches->items[i];
      transfer_counts(match->useq);
   }

}


void
addmatch
(
   useq_t * parent,
   useq_t * child,
   int      distance,
   int      tau
)
{
   // Create stack if not done before.
   if (child->matches == NULL) child->matches = new_tower(tau+1);
   // Create match.
   match_t *match = malloc(sizeof(match_t));
   if (match == NULL) exit(EXIT_FAILURE);
   match->dist = distance;
   match->useq  = parent;
   // Push match to child stack.
   push(match, child->matches);
}


int
mintomax
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *) a;
   useq_t *u2 = (useq_t *) b;
   return (u1->count > u2->count) ? 1 : -1;
}


int
maxtomin
(
   const void *a,
   const void *b
)
{
   return mintomax(b, a);
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
   new->matches = NULL;
   return new;
}


void
destroy_useq
(
   useq_t *useq
)
{
   if (useq->matches != NULL) destroy_tower(useq->matches);
   free(useq->seq);
   free(useq);
}
