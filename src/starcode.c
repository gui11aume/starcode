/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
**
** Last modified: July 8, 2014
**
** License: 
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
*/

#include "starcode.h"
#include "_starcode.h"

FILE *OUTPUT = NULL;

int
starcode
(
   FILE *inputf,
   FILE *outputf,
   const int tau,
   const int verbose,
         int maxthreads,
   const int clusters
)
{
   OUTPUT = outputf;

   if (verbose) fprintf(stderr, "reading input files\n");
   gstack_t *useqS = read_file(inputf);

   // Sort and reduce.
   useqS->nitems = seqsort(useqS->items, useqS->nitems, size_order, maxthreads);

   // Get number of tries.
   int ntries = 3 * maxthreads + (maxthreads % 2 == 0);
   if (useqS->nitems < ntries) {
      ntries = 1;
      maxthreads = 1;
   }
 
   // Pad sequences. (And return the median size)
   int med = -1;
   int height = pad_useq(useqS, &med);
   
   // Make multithreading plan.
   mtplan_t *mtplan = plan_mt(tau, height, med, ntries, useqS, clusters);

   // Run the query.
   run_plan(mtplan, verbose, maxthreads);
   if (verbose) fprintf(stderr, "starcode progress: 100.00%%\n");

   // Remove padding characters.
   unpad_useq(useqS);

   if (clusters == 0) {
      message_passing_clustering(useqS, maxthreads);
   }
   else if (clusters == 1) {
      sphere_clustering(useqS, maxthreads);
   }

   // Do not free anything.
   OUTPUT = NULL;
   return 0;
}

void
run_plan
(
   mtplan_t *mtplan,
   const int verbose,
   const int maxthreads
)
{
   // Count total number of jobs.
   int njobs = mtplan->ntries * (mtplan->ntries+1) / 2;

   // Thread Scheduler
   int triedone = 0;
   int idx = -1;

   while (triedone < mtplan->ntries) { 
      // Cycle through the tries in turn.
      idx = (idx+1) % mtplan->ntries;
      mttrie_t *mttrie = mtplan->tries + idx;
      pthread_mutex_lock(mtplan->mutex);     

      // Check whether trie is idle and there are available threads.
      if (mttrie->flag == TRIE_FREE && mtplan->active < maxthreads) {

         // No more jobs on this trie.
         if (mttrie->currentjob == mttrie->njobs) {
            mttrie->flag = TRIE_DONE;
            triedone++;
         }

         // Some more jobs to do.
         else {
            mttrie->flag = TRIE_BUSY;
            mtplan->active++;
            mtjob_t *job = mttrie->jobs + mttrie->currentjob++;
            pthread_t thread;
            // Start job and detach thread.
            if (pthread_create(&thread, NULL, do_query, job)) {
               fprintf(stderr, "error creating thread (run_plan): %s\n",
                     strerror(errno));
               fprintf(stderr,
                     "Please contact guillaume.filion@gmail.com "
                     "for support with this issue.\n");
               abort();
            }
            pthread_detach(thread);
            if (verbose) {
               fprintf(stderr, "starcode progress: %.2f%% \r",
                     100*(float)(mtplan->jobsdone)/njobs);
            }
         }
      }

      // If max thread number is reached, wait for a thread.
      while (mtplan->active == maxthreads) {
         pthread_cond_wait(mtplan->monitor, mtplan->mutex);
      }

      pthread_mutex_unlock(mtplan->mutex);

   }

   return;

}


void *
do_query
(
   void * args
)  
{
   // Unpack arguments.
   mtjob_t  * job      = (mtjob_t*) args;
   gstack_t * useqS    = job->useqS;
   trie_t   * trie     = job->trie;
   lookup_t * lut      = job->lut;
   int        tau      = job->tau;
   const int  clusters = job->clusters;
   node_t * node_pos   = job->node_pos;

   // Create local hit stack.
   gstack_t **hits = new_tower(tau+1);
   if (hits == NULL) {
      fprintf(stderr, "error creating hit stack (do_query): %s\n",
            strerror(errno));
      fprintf(stderr,
            "Please contact guillaume.filion@gmail.com "
            "for support with this issue.\n");
      abort();
   }

   useq_t * last_query = NULL;
   for (int i = job->start ; i <= job->end ; i++) {
      useq_t *query = (useq_t *) useqS->items[i];
      // Do lookup.
      int do_search = lut_search(lut, query);

      // Insert the new sequence in the lut and trie, but let
      // the last pointer to NULL so that the query does not
      // find itself upon search.
      void **data = NULL;
      if (job->build) {
         lut_insert(lut, query);
         data = insert_string_wo_malloc(trie, query->seq, &node_pos);
         if (data == NULL || *data != NULL) {
            fprintf(stderr, "error building trie (do_query): %s.\n",
                  strerror(errno));
            abort();
            fprintf(stderr,
                  "Please contact guillaume.filion@gmail.com "
                  "for support with this issue.\n");
         }
      }

      if (do_search) {
         int trail = 0;
         if (i < job->end) {
            useq_t *next_query = (useq_t *) useqS->items[i+1];
            // The 'while' condition is guaranteed to be false
            // before the end of the 'char' arrays because all
            // the queries have the same length and are different.
            while (query->seq[trail] == next_query->seq[trail]) {
               trail++;
            }
         }

         // Compute start height.
         int start = 0;
         if (last_query != NULL) {
            while(query->seq[start] == last_query->seq[start]) start++;
         }

         // Clear hit stack.
         for (int j = 0 ; hits[j] != TOWER_TOP ; j++) hits[j]->nitems = 0;
         int err = search(trie, query->seq, tau, hits, start, trail);
         if (err) {
            fprintf(stderr, "search error (do_query): %d\n", err);
            fprintf(stderr,
                  "Please contact guillaume.filion@gmail.com "
                  "for support with this issue.\n");
            abort();
         }

         for (int j = 0 ; hits[j] != TOWER_TOP ; j++) {
            if (hits[j]->nitems > hits[j]->nslots) {
               fprintf(stderr, "warning: incomplete search results (%s)\n",
                       query->seq);
               break;
            }
         }

         // Link matching pairs.
         // Only unique sequences, so start at d=1.
         for (int dist = 1 ; dist < tau+1 ; dist++) {
            for (int j = 0 ; j < hits[dist]->nitems ; j++) {

               useq_t *match = (useq_t *) hits[dist]->items[j];
               // We'll always use parent's mutex.
               useq_t *parent = match->count > query->count ? match : query;
               useq_t *child  = match->count > query->count ? query : match;
               int mutexid;
               if (clusters == 0) {
                  // If clustering is done by message passing, do not link
                  // pair if counts are on the same order of magnitude.
                  int mincount = child->count;
                  int maxcount = parent->count;
                  if (maxcount < PARENT_TO_CHILD_FACTOR * mincount) continue;
                  // The children is modified, use the children mutex.
                  mutexid = match->count > query->count ? job->queryid : job->trieid;
                  pthread_mutex_lock(job->mutex + mutexid);
                  addmatch(child, parent, dist, tau);
                  pthread_mutex_unlock(job->mutex + mutexid);
               }
               else {
                  // The parent is modified, use the parent mutex.
                  mutexid = match->count > query->count ? job->trieid : job->queryid;
                  pthread_mutex_lock(job->mutex + mutexid);
                  addmatch(parent, child, dist, tau);
                  pthread_mutex_unlock(job->mutex + mutexid);
               }
            }
         }

         last_query = query;
      }

      if (job->build) {
         // Finally set the pointer of the inserted tail node.
         *data = query;
      }
   }
   
   destroy_tower(hits);

   // Flag trie, update thread count and signal scheduler.
   // Use the general mutex. (job->mutex[0])
   pthread_mutex_lock(job->mutex);
   *(job->active) -= 1;
   *(job->jobsdone) += 1;
   *(job->trieflag) = TRIE_FREE;
   pthread_cond_signal(job->monitor);
   pthread_mutex_unlock(job->mutex);

   return NULL;

}


mtplan_t *
plan_mt
(
    int       tau,
    int       height,
    int       medianlen,
    int       ntries,
    gstack_t *useqS,
    const int clusters
)
// SYNOPSIS:                                                              
//   The scheduler makes the key assumption that the number of tries is   
//   an odd number, which allows to distribute the jobs among as in the   
//   example shown below. The rows indicate blocks of query strings and   
//   the columns are distinct tries. An circle (o) indicates a build job, 
//   a cross (x) indicates a query job, and a dot (.) indicates that the  
//   block is not queried in the given trie.                              
//                                                                        
//                            --- Tries ---                               
//                            1  2  3  4  5                               
//                         1  o  .  .  x  x                               
//                         2  x  o  .  .  x                               
//                         3  x  x  o  .  .                               
//                         4  .  x  x  o  .                               
//                         5  .  .  x  x  o                               
//                                                                        
//   This simple schedule ensures that each trie is built from one query  
//   block and that each block is queried against every other exactly one 
//   time (a query of block i in trie j is the same as a query of block j 
//   in trie i).                                                          
{
   // Initialize plan.
   mtplan_t *mtplan = malloc(sizeof(mtplan_t));
   if (mtplan == NULL) {
      fprintf(stderr, "out of memory error (plan_mt): %s\n",
            strerror(errno));
      abort();
   }

   // Initialize mutex.
   pthread_mutex_t *mutex = malloc((ntries + 1) * sizeof(pthread_mutex_t));
   pthread_cond_t *monitor = malloc(sizeof(pthread_cond_t));
   if (mutex == NULL || monitor == NULL) {
      fprintf(stderr, "out of memory error (plan_mt): %s\n",
            strerror(errno));
      abort();
   }
   for (int i = 0; i < ntries + 1; i++) pthread_mutex_init(mutex + i,NULL);
   pthread_cond_init(monitor,NULL);

   // Initialize 'mttries'.
   mttrie_t *mttries = malloc(ntries * sizeof(mttrie_t));
   if (mttries == NULL) {
      fprintf(stderr, "out of memory error (plan_mt): %s\n",
            strerror(errno));
      abort();
   }

   // Boundaries of the query blocks.
   int Q = useqS->nitems / ntries;
   int R = useqS->nitems % ntries;
   int *bounds = malloc((ntries+1) * sizeof(int));
   for (int i = 0 ; i < ntries+1 ; i++) bounds[i] = Q*i + min(i, R);

   // Preallocated tries.
   // Count with maxlen-1
   long *nnodes = malloc(ntries * sizeof(long));
   for (int i = 0; i < ntries; i++) nnodes[i] = count_trie_nodes((useq_t **)useqS->items, bounds[i], bounds[i+1]);

   // Create jobs for the tries.
   for (int i = 0 ; i < ntries; i++) {
      // Remember that 'ntries' is odd.
      int njobs = (ntries+1)/2;
      trie_t *local_trie  = new_trie(height);
      node_t *local_nodes = (node_t *) malloc(nnodes[i] * sizeof(node_t));
      mtjob_t *jobs = malloc(njobs * sizeof(mtjob_t));
      if (local_trie == NULL || jobs == NULL) {
         fprintf(stderr, "out of memory error (plan_mt): %s\n",
               strerror(errno));
         abort();
      }

      // Allocate lookup struct.
      // TODO: Try only one lut as well. (It will always return 1 in the query step though).
      lookup_t * local_lut = new_lookup(medianlen, height, tau);
      if (local_lut == NULL) {
         fprintf(stderr, "out of memory error (plan_mt): %s\n",
               strerror(errno));
         abort();
      }

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
         jobs[j].start    = bounds[idx];
         jobs[j].end      = bounds[idx+1]-1;
         jobs[j].tau      = tau;
         jobs[j].clusters = clusters;
         jobs[j].build    = only_if_first_job;
         jobs[j].useqS    = useqS;
         jobs[j].trie     = local_trie;
         jobs[j].node_pos = local_nodes;
         jobs[j].lut      = local_lut;
         jobs[j].mutex    = mutex;
         jobs[j].monitor  = monitor;
         jobs[j].jobsdone = &(mtplan->jobsdone);
         jobs[j].trieflag = &(mttries[i].flag);
         jobs[j].active   = &(mtplan->active);
         // Mutex ids. (mutex[0] is reserved for general mutex)
         jobs[j].queryid  = idx + 1;
         jobs[j].trieid   = i + 1;
      }
   }

   free(bounds);

   mtplan->active = 0;
   mtplan->ntries = ntries;
   mtplan->jobsdone = 0;
   mtplan->mutex = mutex;
   mtplan->monitor = monitor;
   mtplan->tries = mttries;

   return mtplan;

}

long
count_trie_nodes
(
 useq_t ** seqs,
 int     start,
 int     end
)
{
   int  seqlen = strlen(seqs[start]->seq) - 1;
   long count = seqlen;
   for (int i = start+1; i < end; i++) {
      char * a = seqs[i-1]->seq;
      char * b  = seqs[i]->seq;
      int prefix = 0;
      while (a[prefix] == b[prefix]) prefix++;
      count += seqlen - prefix;
   }
   return count;
}


void
sphere_clustering
(
   gstack_t *useqS,
   const int maxthreads
)
{
   // Sort in count order.
   //qsort(useqS->items, useqS->nitems, sizeof(useq_t *), count_order);
   seqsort(useqS->items, useqS->nitems, count_order, maxthreads);

   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *useq = (useq_t *) useqS->items[i];
      if (useq->canonical != NULL) continue;
      useq->canonical = useq;
      if (useq->matches == NULL) continue;

      gstack_t *matches;
      for (int j = 0 ; (matches = useq->matches[j]) != TOWER_TOP ; j++) {
         for (int k = 0 ; k < matches->nitems ; k++) {
            useq_t *match = (useq_t *) matches->items[k];
            match->canonical = useq;
            useq->count += match->count;
            match->count = 0;
         }
      }
   }

   // Sort in count order after update.
   //qsort(useqS->items, useqS->nitems, sizeof(useq_t *), count_order);
   seqsort(useqS->items, useqS->nitems, count_order, maxthreads);

   for (int i = 0 ; i < useqS->nitems ; i++) {

      useq_t *useq = (useq_t *) useqS->items[i];
      if (useq->canonical != useq) break;

      fprintf(OUTPUT, "%s\t", useq->seq);
      if (useq->matches == NULL) {
         fprintf(OUTPUT, "%d\t%s\n", useq->count, useq->seq);
         continue;  
      }

      fprintf(OUTPUT, "%d\t%s", useq->count, useq->seq);

      gstack_t *matches;
      for (int j = 0 ; (matches = useq->matches[j]) != TOWER_TOP ; j++) {
         for (int k = 0 ; k < matches->nitems ; k++) {
            useq_t *match = (useq_t *) matches->items[k];
            fprintf(OUTPUT, ",%s", match->seq);
         }
      }

      fprintf(OUTPUT, "\n");

   }

   return;

}


void
message_passing_clustering
(
   gstack_t *useqS,
   const int maxthreads
)
{
   // Transfer counts to parents recursively.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = (useq_t *) useqS->items[i];
      transfer_counts_and_update_canonicals(u);
   }

   // Sort in canonical order.
   seqsort(useqS->items, useqS->nitems, canonical_order, maxthreads);
   useq_t *first = (useq_t *) useqS->items[0];
   useq_t *canonical = first->canonical;

   // If the first canonical is NULL they all are.
   if (first->canonical == NULL) return;

   fprintf(OUTPUT, "%s\t%d\t%s",
      first->canonical->seq, first->canonical->count, first->seq);

   for (int i = 1 ; i < useqS->nitems ; i++) {
      useq_t *u = (useq_t *) useqS->items[i];
      if (u->canonical == NULL) break;
      if (u->canonical != canonical) {
         canonical = u->canonical;
         fprintf(OUTPUT, "\n%s\t%d\t%s",
               canonical->seq, canonical->count, u->seq);
      }
      else {
         fprintf(OUTPUT, ",%s", u->seq);
      }
   }

   fprintf(OUTPUT, "\n");

   return;

}


int
seqsort
(
 void **data,
 int numels,
 int (*compar)(const void*, const void*),
 int maxthreads
)
// SYNOPSIS:                                                              
//   This function implements a recursive multithreaded mergesort algo-   
//   rithm. The data is provided as an array of pointers and the content  
//   of each array position is passed to the function 'compar()'. The     
//   repeated elements ('compar()' returns 0) are freed (disabled for     
//   speed) from memory and their addresses are set to NULL in data. One  
//   can compute the number of repeats for each element by counting the   
//   preceding number of NULL addresses.                                  
//                                                                        
// PARAMETERS:                                                            
//   data:       an array of pointers to each element.                    
//   numels:     number of elements, i.e. size of 'data'.                 
//   compar:     pointer to the comparison function.                      
//   maxthreads: number of threads.                                       
//                                                                        
// RETURN:                                                                
//   Returns the number of unique elements.                               
//                                                                        
// SIDE EFFECTS:                                                          
//   Pointers to repeated elements are set to NULL.
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

      int i = 0;
      int j = 0;
      int idx = 0;
      int cmp = 0;
      int repeats = 0;

      // Merge sets
      while (i + j < sortargs->size) {
         // Only NULLS at the end of the buffers.
         if (l[i] == NULL && r[j] == NULL) break;
         // Buffers exhausted.
         if (j == arg2.size || r[j] == NULL)      buf[idx++] = l[i++];
         else if (i == arg1.size || l[i] == NULL) buf[idx++] = r[j++];
         // Insert 'NULL' when comparison returns 0.
         else if ((cmp = sortargs->compar(l[i],r[j])) == 0) {
            buf[idx++] = l[i++];
            j++;
            repeats++;
         } 
         // Sort.
         else if (cmp < 0) buf[idx++] = l[i++];
         else              buf[idx++] = r[j++];
      }

      // Pad NULLS.
      sortargs->repeats += repeats;
      for (int k = sortargs->size - sortargs->repeats; k < sortargs->size; k++) buf[k] = NULL;
   }
   
   return NULL;

}


gstack_t *
read_file
(
   FILE *inputf
)
{
   char *seq = NULL;
   char copy[MAXBRCDLEN];
   ssize_t nread;
   size_t nchar = M;
   gstack_t *useqS = new_gstack();
   if (useqS == NULL) {
      fprintf(stderr, "out of memory error (read_file): %s\n",
            strerror(errno));
      abort();
   }
   char *line = malloc(M * sizeof(char));
   if (line == NULL) {
      fprintf(stderr, "out of memory error (read_file): %s\n",
            strerror(errno));
      abort();
   }
   int count = 0;

   // Read sequences from input file.
   while ((nread = getline(&line, &nchar, inputf)) != -1) {
      // Skip fasta header lines.
      if (line[0] == '>') continue;
      // Strip end of line character.
      if (line[nread-1] == '\n') line[nread-1] = '\0';
      if (sscanf(line, "%s\t%d", copy, &count) != 2) {
         count = 1;
         seq = line;
      }
      else {
         seq = copy;
      }
      if (strlen(seq) > MAXBRCDLEN) {
         fprintf(stderr, "max sequence length exceeded (%d) \n", M-1);
         fprintf(stderr, "offending sequence:\n%s\n", seq);
         abort();
      }
      useq_t *new = new_useq(count, seq);
      if (new == NULL) {
         fprintf(stderr, "out of memory error (read_file): %s\n",
               strerror(errno));
         abort();
      }
      push(new, &useqS);
   }

   free(line);
   return useqS;
}


int
pad_useq
(
   gstack_t * useqS,
   int      * median
)
{
   // Compute maximum length.
   int maxlen = 0;
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      if (len > maxlen) maxlen = len;
   }

   // Alloc median bins. (Initializes to 0)
   int  * count = calloc((maxlen + 1), sizeof(int));
   char *spaces = malloc((maxlen + 1) * sizeof(char));
   if (spaces == NULL) {
      fprintf(stderr, "out of memory error (pad_useq): %s\n",
            strerror(errno));
      abort();
   }
   for (int i = 0 ; i < maxlen ; i++) spaces[i] = ' ';
   spaces[maxlen] = '\0';

   // Pad all sequences with spaces.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t *u = useqS->items[i];
      int len = strlen(u->seq);
      count[len]++;
      if (len == maxlen) continue;
      // Create a new sequence with padding characters.
      char *padded = malloc((maxlen + 1) * sizeof(char));
      if (padded == NULL) {
         fprintf(stderr, "out of memory error (pad_useq): %s\n",
               strerror(errno));
         abort();
      }
      memcpy(padded, spaces, maxlen + 1);
      memcpy(padded+maxlen-len, u->seq, len);
      free(u->seq);
      u->seq = padded;
   }

   // Compute median.
   *median = 0;
   int ccount = 0;
   do {
      ccount += count[++(*median)];
   } while (ccount < useqS->nitems / 2);

   // Free and return.
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
      if (unpadded == NULL) {
         fprintf(stderr, "out of memory error (unpad_useq): %s\n",
               strerror(errno));
         abort();
      }
      memcpy(unpadded, u->seq + pad, len - pad + 1);
      free(u->seq);
      u->seq = unpadded;
   }
   return;
}


void
transfer_counts_and_update_canonicals
(
   useq_t *useq
)
{
   // If the read is canonical do nothing.
   if (useq->matches == NULL) {
      useq->canonical = useq;
      return;
   }
   // If the read has already been assigned a canonical, directly
   // transfer counts to the canonical and return.
   if (useq->canonical != NULL) {
      useq->canonical->count += useq->count;
      useq->count = 0;
      return;
   }

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
   for (int i = 0 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      match->count += Q + (i < R);
   }
   // Transfer done.
   useq->count = 0;

   // Continue propagation. This will update the canonicals.
   for (int i = 0 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      transfer_counts_and_update_canonicals(match);
   }

   useq_t *canonical = ((useq_t *) matches->items[0])->canonical;
   for (int i = 1 ; i < matches->nitems ; i++) {
      useq_t *match = (useq_t *) matches->items[i];
      if (match->canonical != canonical) {
         canonical = NULL;
         break;
      }
   }

   useq->canonical = canonical;

   return;

}


void
addmatch
(
   useq_t * from,
   useq_t * to,
   int      dist,
   int      tau
)
{
   if (dist > tau) {
      fprintf(stderr, "error (addmatch): distance exceeds tau.\n");
      fprintf(stderr,
            "Please contact guillaume.filion@gmail.com "
            "for support with this issue.\n");
      abort();
   }
   // Create stack if not done before.
   if (from->matches == NULL) from->matches = new_tower(tau+1);
   push(to, from->matches + dist);
}


lookup_t *
new_lookup
(
 int slen,
 int maxlen,
 int tau
)
{
   lookup_t * lut = (lookup_t *) malloc(2*sizeof(int) + sizeof(int *) +
         (tau+1)*sizeof(char *));
   if (lut == NULL) {
      fprintf(stderr, "error could not create lookup\n");
      return NULL;
   }

   // Target size.
   int k   = slen / (tau + 1);
   int rem = tau - slen % (tau + 1);

   // Set parameters.
   lut->kmers  = tau + 1;
   lut->offset = maxlen - slen;
   lut->klen   = (int *) malloc(lut->kmers * sizeof(int));
   
   // Compute k-mer lengths.
   if (k > MAX_K_FOR_LOOKUP)
      for (int i = 0; i < tau + 1; i++) lut->klen[i] = MAX_K_FOR_LOOKUP;
   else
      for (int i = 0; i < tau + 1; i++) lut->klen[i] = k - (rem-- > 0);

   // Allocate lookup tables.
   for (int i = 0; i < tau + 1; i++) {
      lut->lut[i] = (char *) calloc(1 << max(0,(2*lut->klen[i] - 3)), sizeof(char));
      if (lut->lut[i] == NULL) {
         while (--i > 0) {
            free(lut->lut[i]);
         }  
         free(lut);
         fprintf(stderr, "error could not create lookup\n");
         return NULL;
      }
   }

   return lut;
}


void
destroy_lookup
(
   lookup_t * lut
)
{
   for (int i = 0 ; i < lut->kmers ; i++) free(lut->lut[i]);
   free(lut->klen);
   free(lut);
}


int
lut_search
(
 lookup_t * lut,
 useq_t   * query
)
{
   int offset = lut->offset;
   // Iterate for all kmers and for ins/dels.
   for (int i = 0; i < lut->kmers; i++) {
      for (int j = -i; j <= i; j++) {
         // If sequence contains 'N' seq2id will return -1.
         int seqid = seq2id(query->seq + offset + j, lut->klen[i]);
         if (seqid < 0) continue;
         if ((lut->lut[i][seqid/8] >> (seqid%8)) & 1) return 1;
      }
      offset += lut->klen[i];
   }

   return 0;
}


void
lut_insert
(
 lookup_t * lut,
 useq_t   * query
)
{
   int offset = lut->offset;
   for (int i = 0; i < lut->kmers; i++) {
      int seqid = seq2id(query->seq + offset, lut->klen[i]);
      if (seqid >= 0) lut->lut[i][seqid/8] |= 1 << (seqid%8);
      offset += lut->klen[i];
   }
}


int
seq2id
(
  char * seq,
  int    slen
)
{
   int seqid = 0;
   for (int i = 0; i < slen; i++) {
      // Padding spaces are substituted by 'A'. It does not hurt
      // anyway to generate some false positives.
      if (seq[i] == 'A' || seq[i] == 'a' || seq[i] == ' ') { }
      else if (seq[i] == 'C' || seq[i] == 'c') seqid += 1;
      else if (seq[i] == 'G' || seq[i] == 'g') seqid += 2;
      else if (seq[i] == 'T' || seq[i] == 't') seqid += 3;
      else return -1;
      if (i < slen - 1) seqid <<= 2;
   }

   return seqid;
}


useq_t *
new_useq
(
   int count,
   char *seq
)
{
   useq_t *new = malloc(sizeof(useq_t));
   if (new == NULL) {
      fprintf(stderr, "out of memory error (new_useq): %s\n",
            strerror(errno));
      abort();
   }
   new->seq = malloc((strlen(seq)+1) * sizeof(char));
   strcpy(new->seq, seq);
   new->count = count;
   new->matches = NULL;
   new->canonical = NULL;
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


int
size_order
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *) a;
   useq_t *u2 = (useq_t *) b;
   int la = strlen(u1->seq);
   int lb = strlen(u2->seq);
   if (la == lb) {
      int cmp = strcmp(u1->seq, u2->seq);
      if (cmp == 0) {
         u1->count += u2->count;
         destroy_useq(u2);
      }
      return cmp;
   }
   return (la < lb ? -1 : 1);
}


int
canonical_order
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *) a;
   useq_t *u2 = (useq_t *) b;
   if (u1->canonical == u2->canonical) return strcmp(u1->seq, u2->seq);
   if (u1->canonical == NULL) return 1;
   if (u2->canonical == NULL) return -1;
   if (u1->canonical->count == u2->canonical->count) {
      return strcmp(u1->canonical->seq, u2->canonical->seq);
   }
   if (u1->canonical->count > u2->canonical->count) return -1;
   return 1;
} 


int
count_order
(
   const void *a,
   const void *b
)
{
   useq_t *u1 = (useq_t *) a;
   useq_t *u2 = (useq_t *) b;
   if (u1->count == u2->count) {
      return strcmp(u1->seq, u2->seq);
   }
   return u1->count < u2->count ? 1 : -1;
} 


