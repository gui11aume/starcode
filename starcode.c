#include "starcode.h"
#include "trie.h"

#define str(a) (char *)(a)

useq_t *new_useq(int, char*);
useq_t **get_useq(char**, int, int*, int);
char **read_file(FILE*, int*);
node_t **read_index(FILE *,int,int,int);
void destroy_useq (useq_t*);
void addmatch(useq_t*, useq_t*, int, int);
void transfer_counts(useq_t*);
void unpad_useq (useq_t**, int);
int pad_useq(useq_t**, int);
int abcd(const void *a, const void *b) { return strcmp(str(a), str(b)); }
int ucmp(const useq_t*, const useq_t*);
int ualpha(const void*, const void*);
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
   const int subtrie_level,
   const int mtstrategy
)
{
   OUTPUT = outputf;
   
   if (verbose) fprintf(stderr, "reading input file...");
   int total;
   char **all_seq = read_file(inputf, &total);
   if (verbose) fprintf(stderr, " done\n");

   if (verbose) fprintf(stderr, "preprocessing...");
   int utotal;
   useq_t **all_useq = get_useq(all_seq, total, &utotal, maxthreads);
   free(all_seq);

   int height = pad_useq(all_useq, utotal);
   mergesort((void **) all_useq, utotal, &ualpha, maxthreads);
   if (verbose) fprintf(stderr, " done\n");

   // Compute multithreading plan
   if (verbose) fprintf(stderr,"Computing multithreading plan...");
   mtplan_t *mtplan = starcode_mtplan(tau, height, utotal, subtrie_level, all_useq, mtstrategy);
   if (verbose) fprintf(stderr," done\n");

   // Count total no. of jobs (verbose)
   int njobs = 0;
   if (verbose) {
      for (int t = 0; t < mtplan->numtries; t++)
         njobs +=  mtplan->tries[t].numjobs;
   }

   // Thread Scheduler
   int done = 0;
   int jobsdone = (-mtplan->numtries > -maxthreads ? -mtplan->numtries : -maxthreads);
   int t = 0;
   mttrie_t *mttrie;


   while (done < mtplan->numtries) { 

      // Let's make a fair scheduler.
      t = (t + 1) % mtplan->numtries;

      mttrie = mtplan->tries + t;

      // Check whether the trie is free and there are available threads
      pthread_mutex_lock(mtplan->mutex);     
      if (mttrie->flag == TRIE_FREE && mtplan->threadcount < maxthreads) {

         // When trie is done, count and flag.
         if (mttrie->currentjob == mttrie->numjobs) {
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
            if (pthread_create(&thread, NULL, starcode_thread, (void *) job) != 0) {
               fprintf(stderr, "error: pthread_create (trie no. %d, job no. %d).\n",t,mttrie->currentjob);
               exit(EXIT_FAILURE);
            }
            // Detach thread.
            pthread_detach(thread);

            if (verbose) {
               jobsdone++;
               fprintf(stderr, "Starcode progress: %.2f%% \r", 100*(float)(jobsdone)/njobs);
            }
         }
      }

      while (mtplan->threadcount == min(maxthreads, mtplan->numtries)) {
         pthread_cond_wait(mtplan->monitor, mtplan->mutex);
      }
      pthread_mutex_unlock(mtplan->mutex);
   }


   if (verbose) {
      fprintf(stderr, "Starcode progress: 100.00%%\n");
   }

   unpad_useq(all_useq, utotal);

   // Sort from min to max.
   mergesort((void **)all_useq, utotal, mintomax, maxthreads);

   if (fmt == 0) {
      // Propagate uniformly.
      for (int i = 0 ; i < utotal ; i++) {
         useq_t *useq = all_useq[i];
         transfer_counts(useq);
      }

      // Sort from max to min.
      mergesort((void **)all_useq, utotal, maxtomin, maxthreads);
      for (int i = 0 ; i < utotal ; i++) {
         useq_t *u = all_useq[i];
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
   //   for (int t = 0; t < mtplan->numtries; t++)
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
   mtjob_t  * job      = (mtjob_t*) args;
   useq_t  ** all_useq = job->all_useq;
   node_t   * trie     = job->trie;
   int        tau      = job->tau;

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
      useq_t *query = all_useq[i];

      prefix[0] = '\0';
      int trail = 0;
      if (i < job->end) {
         useq_t *next_query = all_useq[i+1];
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
         node->data = all_useq[i];
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
         if (hits[dist]->nitems == 0) continue;
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
starcode_mtplan
 (
    int      tau,
    int      height,
    int      utotal,
    int      subtrie_level,
    useq_t** useq,
    int      strategy
 )
    // subtrie_level: height at which the subtries start.
 {
    // Initialize plan.
    mtplan_t *plan = (mtplan_t*) malloc(sizeof(mtplan_t));
    plan->threadcount = 0;

    // Initialize mutex
    pthread_mutex_t *mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex,NULL);
    pthread_cond_t *monitor = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(monitor,NULL);

    plan->mutex = mutex;
    plan->monitor = monitor;


    int ntries = 0;
    mtjob_t *bjobs;

    // Compute the first context (build subtries).
    if (strategy == STRATEGY_PREFIX) {
       int nbases = strlen(BASES);
       int maxtries = 1;
       for (int i = 0; i < subtrie_level; i++) maxtries *= nbases;
       bjobs = (mtjob_t *) malloc(maxtries * sizeof(mtjob_t));

       char *prefix = (char *) malloc((subtrie_level+1)*sizeof(char));
       prefix[subtrie_level] = 0;

       for (int i = 0; i < maxtries; i++) {
          int period = 1;
          for (int b = 0; b < subtrie_level; b++) {
             prefix[b] = BASES[(i/period) % nbases];
             period *= nbases;
          }

          // Find prefixes using bisection.
          int start = bisection(0, utotal-1, prefix, useq, subtrie_level, BISECTION_START);
          if (strncmp(prefix, useq[start]->seq, subtrie_level) != 0) continue;
          int end = bisection(0, utotal-1, prefix, useq, subtrie_level, BISECTION_END);

          // Fill mtjob.
          bjobs[i].start       = start;
          bjobs[i].end         = end;
          bjobs[i].tau         = tau;
          bjobs[i].build       = 1;
          bjobs[i].all_useq    = useq;
          bjobs[i].trie        = new_trie(tau,height);
          bjobs[i].mutex       = mutex;
          bjobs[i].monitor     = monitor;
          bjobs[i].threadcount = &(plan->threadcount);

          ntries++;
       }

    }
    else {
       ntries = subtrie_level;
       bjobs = (mtjob_t *) malloc(ntries * sizeof(mtjob_t));
       int start = 0, end, offset;
       offset = utotal/ntries;
       end = offset - 1;

       for (int i=0; i < ntries; i++) {

          if (i == ntries - 1) end = utotal - 1;

          bjobs[i].start       = start;
          bjobs[i].end         = end;
          bjobs[i].tau         = tau;
          bjobs[i].build       = 1;
          bjobs[i].all_useq    = useq;
          bjobs[i].trie        = new_trie(tau,height);
          bjobs[i].mutex       = mutex;
          bjobs[i].monitor     = monitor;
          bjobs[i].threadcount = &(plan->threadcount);

          start += offset;
          end   += offset;
       }
    }


    // Compute number of jobs and query contexts.
    int totaljobs = 0;
    for (int i = ntries; i > 0; i--) totaljobs += i;
    int ncont = totaljobs/ntries + (totaljobs%ntries > 0);

    // Initialize mttries.
    plan->tries = (mttrie_t *) malloc(ntries * sizeof(mttrie_t));
    plan->numtries = ntries;
    for (int t=0; t < ntries; t++) {
       // Initialize mttrie.
       plan->tries[t].flag       = TRIE_FREE;
       plan->tries[t].currentjob = 0;
       plan->tries[t].numjobs    = 1;
       plan->tries[t].jobs       = (mtjob_t *) malloc(ncont * sizeof(mtjob_t));
       // Add build job to the 1st position.
       memcpy(plan->tries[t].jobs, bjobs + t, sizeof(mtjob_t));
       // Assign trie flag pointer
       plan->tries[t].jobs[0].trieflag = &(plan->tries[t].flag);
    }

    // Compute query contexts.
    int njobs = ntries;
    for (int c = 1; c < ncont; c++) {
       if (c == ncont - 1) {
          // Fill the remaining query combinations.
          njobs = totaljobs % ntries;
          if (njobs == 0) njobs = ntries;
       }
       // Assign query jobs to each trie.
       // njobs contains the number of jobs (i.e. tries that will be queried) in the current context.
       for (int j = 0; j < njobs; j++) {
          plan->tries[j].numjobs++;
          mtjob_t * job = plan->tries[j].jobs + c;

          // Fill mtjob.
          job->tau         = tau;
          job->mutex       = mutex;
          job->monitor     = monitor;
          job->trieflag    = &(plan->tries[j].flag);
          job->threadcount = &(plan->threadcount);
          // Query seq. shifted 'c' positions.
          job->all_useq    = useq;
          job->start       = bjobs[(j+c)%ntries].start;
          job->end         = bjobs[(j+c)%ntries].end;
          // Trie to query.
          job->trie        = bjobs[j].trie;
          // Do not build, just query.
          job->build       = 0;
       }
    }

    free(bjobs);

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
 void ** data,
 int numels,
 int (*compar)(const void*, const void*),
 int maxthreads
)
{
   // Copy to buffer.
   void ** buffer = (void **)malloc(numels * sizeof(void *));
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

   _mergesort((void *) &args);

   free(buffer);
   
   return numels - args.repeats;
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
   all_seq = realloc(all_seq, *nlines * sizeof(char *));

   free(seq);
   return all_seq;
}



node_t **
read_index
(
   FILE *input,
   int ntries,
   int tau,
   int height
)
{
   node_t ** tries = (node_t **) malloc(ntries*sizeof(node_t *));

   ssize_t nread;
   size_t nchar = MAXBRCDLEN;
   char *seq = malloc(MAXBRCDLEN * sizeof(char));
   // First read the number of unique sequences.
   nread = getline(&seq, &nchar, input);
   if (seq[nread-1] == '\n') seq[nread-1] = '\0';

   int nseq = atoi(seq);
   if (nseq < 1) {
      fprintf(stderr,"Error reading index file: bad format.\n");
      exit(EXIT_FAILURE);
   }
   int period = nseq / ntries;

   // Read sequences from input file and store in an array. Assume
   // that it contains one sequence per line and nothing else. 
   int i = 0;
   while ((nread = getline(&seq, &nchar, input)) != -1) {
      // Read sequence.
      if (seq[nread-1] == '\n') seq[nread-1] = '\0';
      char *new = malloc(nread);
      if (new == NULL) exit(EXIT_FAILURE);
      strncpy(new, seq, nread);

      // Build new trie if necessary.
      if (i % period == 0) tries[i/period] = new_trie(tau, height);

      // Insert sequence.
      node_t * trie = tries[i/period];
      node_t * node = insert_string(trie, new);
      if (node == NULL) {
         fprintf(stderr,"Error building index trie.\n");
         exit(EXIT_FAILURE);
      }
      if (node != trie) node->data = new_useq(0, new);
      i++;
   }

   free(seq);
   return tries;
}




useq_t **
get_useq
(
   char ** all_seq,
   int     total,
   int  *  utotal,
   int     maxthreads
)
{
   // Sort sequences, count and compact them. Sorting
   // alphabetically is important for speed.
   *utotal = mergesort((void **)all_seq, total, abcd, maxthreads);
   useq_t **all_useq = malloc((*utotal) * sizeof(useq_t *));
   
   int ucount = 0;
   int k = 0;
   for (int i = 0 ; i < total; i++) {
      ucount++;
      if (all_seq[i] != NULL) {
         all_useq[k++] = new_useq(ucount, all_seq[i]);
         ucount = 0;
      }
   }

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
   for (int i = 0; i < matches->nitems; i++) {
      match_t *match = (match_t *) matches->items[i];
      match->useq->count += Q + (i < R);
   }

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
   push(new_match(parent, distance), child->matches);
}


match_t *
new_match
(
   useq_t * parent,
   int      distance
)
{
   match_t * match = malloc(sizeof(match_t));
   if (match == NULL) exit(EXIT_FAILURE);
   match->dist = distance;
   match->useq  = parent;
   return match;
}


int
maxtomin
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *)a;
   useq_t *u2 = (useq_t *)b;
   return ucmp(u2, u1);
}

int
mintomax
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *)a;
   useq_t *u2 = (useq_t *)b;
   if (u1->count > u2->count) return 1;
   else return -1;
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
   useq_t *u1 = (useq_t *)a;
   useq_t *u2 = (useq_t *)b;
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
