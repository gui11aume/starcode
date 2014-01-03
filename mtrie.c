#include "mtrie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// Macro used for readbility of the code.
#define child_is_a_hit (depth+tau >= query[0]) && (child->data != NULL) \
            && (ccache[depth+query[0]*M] <= tau)

// Module error reporting.
int ERROR;

// Array of nodes for the hits.
narray_t *HITS = NULL;


// Local functions.
node_t *minsert(node_t*, int, char);
void recursive_msearch(node_t*, int*, int, int, const char, narray_t**, int);
void dash(node_t*, int*);
void push(node_t*, narray_t*);
void init_cache(node_t*);
int get_maxtau(node_t *root) { return ((minfo_t *)root->data)->maxtau; }
int check_trie_error_and_reset(void);


// ------  SEARCH  ------ //


narray_t *
msearch
(
   node_t *mtrie,
   const char *query,
   int tau,
   narray_t *hits,
   int startat,
   int trailto
)
{
   
   char maxtau = get_maxtau(mtrie);
   if (tau > maxtau) {
      ERROR = 48;
      // DETAIL: the nodes' cache has been allocated just enough
      // space to hold Levenshtein distances up to a maximum tau.
      // If this is exceeded, the search will try to read from and
      // write to forbidden memory space.
      return hits;
   }

   int length = strlen(query);
   if (length > MAXBRCDLEN) {
      ERROR = 53;
      return hits;
   }

   // Make sure the cache is warm and waiting.
   minfo_t *info = (minfo_t *) mtrie->data;
   if (*info->cache == NULL) init_cache(mtrie);

   // Reset the cache levels that will be overwritten.
   for (int i = startat+1 ; i < trailto ; i++) info->cache[i]->idx = 0;

   // Translate the query string. The first 'char' is kept to store
   // the length of the query, which shifts the array by 1 position.
   int translated[M];
   translated[0] = length;
   for (int i = startat ; i < length ; i++) {
      translated[i+1] = translate[(int) query[i]];
   }
   // XXX Is this needed? XXX //
   //translated[length] = EOS;

   // Run recursive search from cached nodes.
   narray_t *cache = info->cache[startat];
   for (int i = 0 ; i < cache->idx ; i++) {
      recursive_msearch(cache->nodes[i], translated, tau, startat + 1,
            maxtau, info->cache, trailto);
   }

   return HITS;

}


void
recursive_msearch
(
   node_t *node,
   int *query,
   int tau,
   int depth,
   const char maxtau,
   narray_t **mcache,
   int trailto
)
{

   if (depth > query[0] + tau) return;

   node_t *child;
   char *pcache = node->cache + maxtau + 2;

   // TODO: check boundary condition to not overshoot.
   int maxa = min(depth - 1, tau);
   for (int i = 0 ; i < 4 ; i++) {

      // Skip if current node has no child at this position.
      if ((child = node->child[i]) == NULL) continue;

      char path = child->cache[0];
      char *ccache = child->cache + maxtau + 2;

      int mindist = INT_MAX;
      int mmatch;
      int shift;

      // Fill in an angle of the dynamic programming table.
      for (int a = maxa ; a > 0 ; a--) {
         // Right side (need the path).
         int mmatch = pcache[a] + ((path >> (2*a) & 3) != query[depth]);
         int shift = min(pcache[a-1], ccache[a+1]) + 1;
         ccache[a] = min(mmatch, shift);
         if (ccache[a] < mindist) mindist = ccache[a];
      }
      for (int a = maxa ; a >= 0 ; a--) {
         // Left side and center.
         int mmatch = pcache[-a] + (i != query[depth-a]);
         int shift = min(pcache[1-a], ccache[-a-1]) + 1;
         ccache[-a] = min(mmatch, shift);
         if (ccache[-a] < mindist) mindist = ccache[-a];
      }

      // Early exit if tau is exceeded.
      if (mindist > tau) return;

      if (trailto <= depth) push(child, mcache[depth]);

      // In case the smallest Levenshtein distance is
      // equal to the maximum allowed distance, no more mismatches
      // and indels are allowed. We can shortcut by searching perfect
      // matches.

      /*
      if (mindist == tau) {
         int left = max(depth-tau, 0);
         for (int a = left ; a < maxa ; a++) {
            if (DYNP[(depth)+(a)*M] == mindist) {
               dash(child, query+a);
            }
         }
         continue;
      }
      */

      // We have a hit if the child node is a tail, and the
      // Levenshtein distance is not larger than 'tau'. But first
      // we need to make sure that the Levenshtein distance to the
      // query has been computed, which is the first condition of the
      // macro used below.
      // 1. depth+tau >= limit
      // 2. child->data != NULL
      // 3. DYNP[depth+limit*M] <= tau
      if (child_is_a_hit) {
         push(child, HITS);
      }

      recursive_msearch(child, query, tau, depth+1, maxtau, mcache, trailto);

   }

}


void
dash
(
   node_t *node,
   int *query
)
{
   int c;
   node_t *child;

   while ((c = *query++) != EOS) {
      if ((c > 4) || (child = node->child[c]) == NULL) return;
      node = child;
   }

   if (node->data != NULL) {
      push(node, HITS);
   }
   return;
}


void
push
(
   node_t *node,
   narray_t *stack
)
{
   if (stack->idx >= stack->lim) {
      size_t newsize = 2 * sizeof(int) + 2*stack->lim * sizeof(node_t *);
      narray_t *ptr = realloc(stack, newsize);
      if (ptr == NULL) {
         ERROR = 175;
         return;
      }
      stack = ptr;
      stack->lim *= 2;
   }
   stack->nodes[stack->idx++] = node;
}


// ------  TRIE CONSTRUCTION  ------ //


node_t *
insert_string
(
   node_t *root,
   const char *string
)
// SYNOPSIS:                                                              
//   Helper function to construct tries. Insert a string from a root node 
//   by multiple calls to 'insert_char'.                                  
//                                                                        
// RETURN:                                                                
//   The leaf node in case of succes, 'NULL' otherwise.                   
{
   int nchar = strlen(string);
   if (nchar > MAXBRCDLEN) {
      ERROR = 204;
      return NULL;
   }
   
   char maxtau = get_maxtau(root);
   int i;

   // Find existing path and append one node.
   node_t *child;
   for (i = 0 ; i < nchar ; i++) {
      int c = translate[(int) string[i]];
      if ((child = child->child[c]) == NULL) {
         if (c < 5) child = minsert(child, c, maxtau);
         break;
      }
   }

   // Append more nodes.
   for (i++ ; i < nchar ; i++) {
      if (child == NULL) {
         ERROR = 228;
         return NULL;
      }
      int c = translate[(int) string[i]];
      if (c < 5) child = minsert(child, c, maxtau);
   }

   return child;

}


node_t *
minsert
(
   node_t *parent,
   int c,
   char maxtau
)
// SYNOPSIS:                                                              
//   Helper function to construct tries. Append a child to an existing    
//   node at a position specified by the character 'c'. NO CHECKING IS    
//   PERFORMED to make sure that this does not overwrite an existings     
//   node child (causing a memory leak) or that 'c' is an integer less    
//   than 5. Since 'minsert' is called exclusiverly by 'insert_string'    
//   after a call to 'find_path', this checking is not required. If       
//   'insert' is called in another context, this check has to be          
//   performed.                                                           
//                                                                        
// RETURN:                                                                
//   The appended child node in case of success, 'NULL' otherwise.        
{
   // Initilalize child node.
   node_t *child = new_mtrienode(maxtau);
   if (child == NULL) {
      ERROR = 254;
      return NULL;
   }
   // Update child path.
   child->cache[0] = (parent->cache[0] << 2) + c;
   // Update parent node.
   parent->child[c] = child;
   return child;
}


// ------  CONSTRUCTORS and DESTRUCTORS  ------ //


narray_t *
new_narray
(void)
{
   narray_t *new = malloc(2 * sizeof(int) + 32 * sizeof(node_t *));
   if (new == NULL) {
      ERROR = 279;
      return NULL;
   }
   new->idx = 0;
   new->lim = 32;
   return new;
}


node_t *
new_mtrie
(
   char maxtau
)
{

   if (maxtau > 8) {
      ERROR = 337;
      // DETAIL:                                                         
      // There is an absolute limit at 'tau' = 8 because the path is     
      // encoded as a 'char', ie an 8 x 2-bit array. It should be enough 
      // for most practical purposes.                                    
      return NULL;
   }

   node_t *root = new_mtrienode(maxtau);
   if (root == NULL) {
      ERROR = 347;
      return NULL;
   }
   minfo_t *info = malloc(sizeof(minfo_t));
   if (info == NULL) {
      ERROR = 294;
      return NULL;
   }
   memset(info->cache, 0, M * sizeof(narray_t *));
   info->maxtau = maxtau;
   return root;
}


node_t *
new_mtrienode
(
   char maxtau
)
// SYNOPSIS:                                                              
//   Constructor for a trie node or a  root.                              
//                                                                        
// RETURN:                                                                
//   A root node with no data and no children.                            
{
   size_t base = sizeof(node_t);
   size_t extra = (2*maxtau + 4) * sizeof(char);
   node_t *node = malloc(sizeof(base + extra));
   if (node == NULL) {
      ERROR = 331;
      return NULL;
   }
   memset(node, 0, base);
   // Save first 'char' for the trie path.
   for (char i = 1 ; i < 2*maxtau + 4 ; i++) node->cache[i] = i-2-maxtau;
   return node;
}


void
destroy_mtrie
(
   node_t *mtrie,
   void (*destruct)(void *)
)
{
   // Free the node arrays in the cache.
   minfo_t *info = (minfo_t *) mtrie->data;
   for (int i = 0 ; i < M ; i++) free(info->cache[i]);
   // Set info to 'NULL' before recursive destruction.
   free(info);
   info = NULL;
   // ... and bye-bye.
   destroy_nodes_downstream_of(mtrie, destruct);
}


void
destroy_nodes_downstream_of
(
   node_t *node,
   void (*destruct)(void *)
)
// SYNOPSIS:                                                              
//   Free the memory allocated on a trie.                                 
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{  
   if (node != NULL) {
      for (int i = 0 ; i < 5 ; i++) {
         destroy_nodes_downstream_of(node->child[i], destruct);
      }
      if (node->data != NULL && destruct != NULL) (*destruct)(node->data);
      free(node);
   }
}


// ------  MISCELLANEOUS ------ //

void init_cache
(
   node_t *mtrie
)
{
   minfo_t *info = (minfo_t *) mtrie->data;
   for (int i = 0 ; i < M ; i++) {
      info->cache[i] = new_narray();
      // TODO: You can do better than this!
      if (info->cache[i] == NULL) exit (EXIT_FAILURE);
   }
   // Push the root into the 0-depth cache.
   // It will be the only node ever in there.
   push(mtrie, info->cache[0]);
}


int check_trie_error_and_reset(void) {
   if (ERROR) {
      int last_error_at_line = ERROR;
      ERROR = 0;
      return last_error_at_line;
   }
   return 0;
}
