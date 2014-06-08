#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// Globals.
int ERROR = 0;

// Here functions.
int get_maxtau(trie_t *trie) { return trie->info->maxtau; }
int get_height(trie_t *trie) { return trie->info->height; }

// --  DECLARATION OF PRIVATE FUNCTIONS  -- //

// Search.
void poucet(node_t*, int, struct arg_t);
void dash(node_t*, const int*, struct arg_t);
// Trie creation and destruction.
node_t *insert(node_t*, int, unsigned char);
node_t *new_trienode(unsigned char);
void destroy_nodes_recursively(node_t*, void(*)(void*), int, int);
// Snippets.
int recursive_count_nodes(node_t * node, int, int);


// ------  SEARCH FUNCTIONS ------ //

int
search
(
         trie_t   *  trie,
   const char     *  query,
   const int         tau,
//         hstack_t ** hits,
         gstack_t ** hits,
         int         start,
   const int         trail
)
// SYNOPSIS:                                                              
//   Front end query of a trie with the "trail search" algorithm. Search  
//   does not start from root. Instead, it starts from a given depth      
//   corresponding to the length of the prefix from the previous search.  
//   Since initial computations are identical for queries starting with   
//   the same prefix, the search can restart from there. Each call to     
//   the function starts ahead and leaves a trail for the next query.     
//                                                                        
// PARAMETERS:                                                            
//   trie: the trie to query                                              
//   query: the query as an ascii string                                  
//   tau: the maximum edit distance                                       
//   hits: a hit stack to push the hits                                   
//   start: the depth to start the search                                 
//   trail: how deep to trail                                             
//                                                                        
// RETURN:                                                                
//   A pointer to 'hits' node array.                                      
//                                                                        
// SIDE EFFECTS:                                                          
//   The non 'const' parameters of the function are modified. The node    
//   array can be resized (which is why the address of the pointers is    
//   passed as a parameter) and the trie is modified by the trailinng of  
//   effect of the search.                                                
{
   ERROR = 0;

   char maxtau = get_maxtau(trie);
   char height = get_height(trie);
   if (tau > maxtau) {
      // DETAIL: the nodes' cache has been allocated just enough
      // space to hold Levenshtein distances up to a maximum tau.
      // If this is exceeded, the search will try to read from and
      // write to forbidden memory space.
      fprintf(stderr, "error: requested tau greater than 'maxtau'\n");
      return 74;
   }

   int length = strlen(query);
   if (length > height) {
      fprintf(stderr, "error: query longer than allowed max\n");
      return 80;
   }

   // Make sure the cache is allocated.
   info_t *info = trie->info;

   // Reset the milestones that will be overwritten.
   start = max(start, 0);
   for (int i = start+1 ; i <= min(trail, height) ; i++) {
      info->milestones[i]->nitems = 0;
   }

   // Translate the query string. The first 'char' is kept to store
   // the length of the query, which shifts the array by 1 position.
   int translated[M];
   translated[0] = length;
   translated[length+1] = EOS;
   for (int i = max(0, start-maxtau) ; i < length ; i++) {
      translated[i+1] = altranslate[(int) query[i]];
   }

   // Set the search options.
   struct arg_t arg = {
      .hits        = hits,
      .query       = translated,
      .tau         = tau,
      .maxtau      = maxtau,
      .milestones  = info->milestones,
      .trail       = trail,
      .height      = height,
   };

   // Run recursive search from cached nodes.
   gstack_t *milestones = info->milestones[start];
   for (int i = 0 ; i < milestones->nitems ; i++) {
      node_t *start_node = (node_t *) milestones->items[i];
      poucet(start_node, start + 1, arg);
   }

   // Return the error code of the process (the line of
   // the last error) and 0 if everything went OK.
   return check_trie_error_and_reset();

}


void
poucet
(
          node_t * restrict node,
   const  int      depth,
   struct arg_t    arg
)
// SYNOPSIS:                                                              
//   Back end recursive "trail search" algorithm. Most of the time is     
//   spent in this function. The focus node sets the values for an L-     
//   shaped section (but with the angle on the right side) of the dynamic 
//   programming table for its children. One of the arms of the L is      
//   identical for all the children and is calculated separately. The     
//   rest is classical dynamic programming computed in the 'cache' struct 
//   member of the children. The path of the last 8 nodes leading to the  
//   focus node is encoded by a 32 bit integer, which allows to perform   
//   dynamic programming without parent pointer.                          
//   The tails of the nodes are constrained to be leaves so that tails    
//   and queries have the same length. This is done by adding a "magic"   
//   node (in position 5) corresponding to a dummy prefix (printed as a   
//   white space). All the leaves of the trie are at a depth called the   
//   "height", which is the depth at which the recursion is stopped to    
//   check for hits. If the maximum edit distance 'tau' is exceeded the   
//   search is interrupted. On the other hand, if the search has passed   
//   trailing depth and 'tau' is exactly reached, the search finishes by  
//   a 'dash()' which checks whether an exact suffix can be found.        
//   While trailing, the nodes are pushed in milestone node arrays that   
//   can be serve as starting points for future searches.                 
//                                                                        
// PARAMETERS:                                                            
//   node: the focus node in the trie                                     
//   depth: the depth of the children in the trie.                        
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Same as 'search()', it modifies nodes of the trie and the node       
//   array 'arg.hits' where hits are pushed.                              
{

   // This makes it easier to distinguish the part that goes upward,
   // with positive index and requiring the path, from the part that
   // goes horizontally, with negative index and requiring previous
   // characters of the query.
   char *pcache = node->cache + arg.maxtau;
   // Risk of overflow at depth lower than 'tau'.
   int maxa = min((depth-1), arg.tau);

   // Penalty for match/mismatch and insertion/deletion resepectively.
   unsigned char mmatch;
   unsigned char shift;

   // Part of the cache that is shared between all the children.
   char common[9] = {1,2,3,4,5,6,7,8,9};

   // The branch of the L that is identical among all children
   // is computed separately. It will be copied later.
   uint32_t path = node->path;
   // Upper arm of the L (need the path).
   if (maxa > 0) {
      // Special initialization for first character. If the previous
      // character was a PAD, there is no cost to start the alignment.
      mmatch = (arg.query[depth-1] == PAD ? 0 : pcache[maxa]) +
                  ((path >> 4*(maxa-1) & 15) != arg.query[depth]);
      shift = min(pcache[maxa-1], common[maxa]) + 1;
      common[maxa-1] = min(mmatch, shift);
      for (int a = maxa-1 ; a > 0 ; a--) {
         mmatch = pcache[a] + ((path >> 4*(a-1) & 15) != arg.query[depth]);
         shift = min(pcache[a-1], common[a]) + 1;
         common[a-1] = min(mmatch, shift);
      }
   }

   node_t *child;
   for (int i = 0 ; i < 6 ; i++) {
      // Skip if current node has no child at this position.
      if ((child = (node_t *) node->child[i]) == NULL) continue;

      // Same remark as for parent cache.
      char local_cache[] = {9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8,9};
      char *ccache = depth == arg.height ?
         local_cache + 9 : child->cache + arg.maxtau;
      memcpy(ccache+1, common, arg.maxtau * sizeof(char));

      // Horizontal arm of the L (need previous characters).
      if (maxa > 0) {
         // See comment above for initialization.
         mmatch = ((path & 15) == PAD ? 0 : pcache[-maxa]) +
                     (i != arg.query[depth-maxa]);
         shift = min(pcache[1-maxa], maxa+1) + 1;
         ccache[-maxa] = min(mmatch, shift);
         for (int a = maxa-1 ; a > 0 ; a--) {
            mmatch = pcache[-a] + (i != arg.query[depth-a]);
            shift = min(pcache[1-a], ccache[-a-1]) + 1;
            ccache[-a] = min(mmatch, shift);
         }
      }
      // Center cell (need both arms to be computed).
      mmatch = pcache[0] + (i != arg.query[depth]);
      shift = min(ccache[-1], ccache[1]) + 1;
      ccache[0] = min(mmatch, shift);

      // Stop searching if 'tau' is exceeded.
      if (ccache[0] > arg.tau) continue;

      // Reached height of the trie: it's a hit!
      if (depth == arg.height) {
         push(child, arg.hits + ccache[0]);
         continue;
      }

      // Cache nodes in milestones when trailing.
      if (depth <= arg.trail) push(child, (arg.milestones)+depth);

      if (depth > arg.trail) {
         // Use 'dash()' if no more mismatches allowed.
         int can_dash = 1;
         for (int a = -maxa ; a < maxa+1 ; a++) {
            if (ccache[a] < arg.tau) {
               can_dash = 0;
               break;
            }
         }
         if (can_dash) {
            dash(child, arg.query+depth+1, arg);
            continue;
         }
      }

      poucet(child, depth+1, arg);

   }

}


void
dash
(
          node_t * restrict node,
   const  int    * restrict suffix,
   struct arg_t    arg
)
// SYNOPSIS:                                                              
//   Checks whether node has the given suffix and reports a hit if this   
//   is the case.                                                         
//                                                                        
// PARAMETERS:                                                            
//   node: the node to test                                               
//   suffix: the suffix to test as a translated sequence                  
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Updates 'arg.hits' if the suffix is found.                           
{

   int c;
   node_t *child;

   // Early return if the suffix path is broken.
   while ((c = *suffix++) != EOS) {
      if ((c > 4) || (child = (node_t *) node->child[c]) == NULL) return;
      node = child;
   }

   // End of query, check whether node is a tail.
   push(node, arg.hits + arg.tau);

   return;

}


// ------  TRIE CONSTRUCTION AND DESTRUCTION  ------ //


trie_t *
new_trie
(
   unsigned char maxtau,
   unsigned char height
)
// SYNOPSIS:                                                              
//   Front end trie constructor.                                          
//                                                                        
// PARAMETERS:                                                            
//   maxtau: the max value that tau can take in a search                  
//   height: the fixed depth of the leaves                                
//                                                                        
// RETURN:                                                                
//   A pointer to trie root with meta information and no children.        
{

   if (maxtau > 8) {
      fprintf(stderr, "error: 'maxtau' cannot be greater than 8\n");
      ERROR = 323;
      // DETAIL:                                                         
      // There is an absolute limit at 'tau' = 8 because the struct      
      // member 'path' is encoded as a 32 bit 'int', ie an 8 x 4-bit     
      // array. It should be enough for most practical purposes.         
      return NULL;
   }

   trie_t *trie = malloc(sizeof(trie_t *));
   if (trie == NULL) {
      fprintf(stderr, "error: could not create trie\n");
      ERROR = 334;
      return NULL;
   }

   node_t *root = new_trienode(maxtau);
   if (root == NULL) {
      fprintf(stderr, "error: could not create root\n");
      ERROR = 341;
      return NULL;
   }

   info_t *info = malloc(sizeof(info_t));
   if (info == NULL) {
      fprintf(stderr, "error: could not create trie\n");
      ERROR = 348;
      free(root);
      return NULL;
   }

   // Set the values of the meta information.
   info->maxtau = maxtau;
   info->height = height;
   info->milestones = new_tower(M);

   if (info->milestones == NULL) {
      fprintf(stderr, "error: could not create trie\n");
      ERROR = 360;
      free(info);
      free(root);
      return NULL;
   }

   push(root, info->milestones);
   trie->root = root;
   trie->info = info;

   return trie;

}


node_t *
new_trienode
(
   unsigned char maxtau
)
// SYNOPSIS:                                                              
//   Back end constructor for a trie node. All values are initialized to  
//   null, except the cache for dynamic programming which is initialized  
//   as a root node.                                                      
//                                                                        
// PARAMETERS:                                                            
//   maxtau: the max value that tau can take in a search                  
//                                                                        
// RETURN:                                                                
//   A pointer to trie node with no data and no children.                 
{
   size_t cachesize = (2*maxtau + 1) * sizeof(char);
   node_t *node = malloc(sizeof(node_t) + cachesize);
   if (node == NULL) {
      fprintf(stderr, "error: could not create trie node\n");
      ERROR = 395;
      return NULL;
   }

   // Set all base values to 0.
   node->path = 0;
   for(int i = 0; i < 6; i++) node->child[i] = NULL;
   // Initialize the cache. This is important for the
   // dynamic programming algorithm.
   const char init[17] = {8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8};
   memcpy(node->cache, init+(8-maxtau), cachesize);
   return node;
}


void **
insert_string
(
         trie_t * trie,
   const char   * string
)
// SYNOPSIS:                                                              
//   Front end function to fill in a trie. Insert a string from root, or  
//   simply return the node at the end of the string path if it already   
//   exists.                                                              
//                                                                        
// RETURN:                                                                
//   The leaf node in case of succes, 'NULL' otherwise.                   
{

   int i;

   int nchar = strlen(string);
   if (nchar > get_height(trie)) {
      fprintf(stderr, "error: cannot insert string longer than %d\n",
            get_height(trie));
      ERROR = 432;
      return NULL;
   }
   
   unsigned char maxtau = get_maxtau(trie);

   // Find existing path.
   node_t *node = trie->root;
   for (i = 0 ; i < nchar-1; i++) {
      node_t *child;
      int c = translate[(int) string[i]];
      if ((child = (node_t *) node->child[c]) == NULL) {
         break;
      }
      node = child;
   }

   // Append more nodes.
   for ( ; i < nchar-1 ; i++) {
      int c = translate[(int) string[i]];
      node = insert(node, c, maxtau);
      if (node == NULL) {
         fprintf(stderr, "error: could not insert string\n");
         ERROR = 455;
         return NULL;
      }
   }

   return node->child + translate[(int) string[nchar-1]];

}


node_t *
insert
(
            node_t * parent,
            int      position,
   unsigned char     maxtau
)
// SYNOPSIS:                                                              
//   Back end function to construct tries. Append a child to an existing  
//   node at specifieid position.                                         
//   NO CHECKING IS PERFORMED to make sure that this does not overwrite   
//   an existings node child (causing a memory leak) or that 'c' is an    
//   integer less than 5. Since 'insert' is called exclusiverly by        
//   'insert_string' after a call to 'find_path', this checking is not    
//   required. If 'insert' is called in another context, this check has   
//   to be performed.                                                     
//                                                                        
// PARAMETERS:                                                            
//   parent: the parent to append the node to                             
//   position: the position of the child                                  
//   maxtau: the maximum value of tau used for searches                   
//                                                                        
// RETURN:                                                                
//   The appended child node in case of success, 'NULL' otherwise.        
{
   // Initilalize child node.
   node_t *child = new_trienode(maxtau);
   if (child == NULL) {
      fprintf(stderr, "error: could not insert node\n");
      ERROR = 494;
      return NULL;
   }
   // Update child path and parent pointer.
   child->path = (parent->path << 4) + position;
   // Update parent node.
   parent->child[position] = child;

   return child;

}


void
destroy_trie
(
   trie_t *trie,
   void (*destruct)(void *)
)
// SYNOPSIS:                                                              
//   Front end function to recycle the memory allocated to a trie. Node   
//   data may or may not be recycled as well, depending on the destructor 
//   passed as argument. This function is essentially a wrapper for       
//   'destroy_nodes_downstream_of()', the only difference is that it      
//   destroys the meta-data of the trie first.                            
//                                                                        
// PARAMETERS:                                                            
//   trie: the trie to destroy                                            
//   destruct: a function with the same prototype as 'free()'             
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Frees the memory allocated to the nodes of a trie, the meta-data     
//   associated to the root, and possibly the data associated to the tail 
//   nodes.                                                               
{
   // Free the milesones.
   destroy_tower(trie->info->milestones);
   destroy_nodes_recursively(trie->root, destruct, get_height(trie), 0);
   free(trie->info);
   free(trie);
}


void
destroy_nodes_recursively
(
   node_t *node,
   void (*destruct)(void *),
   int maxdepth,
   int depth
)
// SYNOPSIS:                                                              
//   Back end function to free the memory allocated on a trie. It should  
//   not be called directly on a trie to prevent memory leaks (otherwise  
//   the milestones will not be freed).                                   
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Frees the memory allocated to the nodes of a trie, and possibly the  
//   data associated to the tail nodes.                                   
{  
   if (node != NULL) {
      if (depth == maxdepth) {
         if (destruct != NULL) (*destruct)(node);
         return;
      }
      for (int i = 0 ; i < 6 ; i++) {
         node_t * child = (node_t *) node->child[i];
         destroy_nodes_recursively(child, destruct, maxdepth, depth+1);
      }
      free(node);
   }
   return;
}



// ------  UTILITY FUNCTIONS ------ //


gstack_t *
new_gstack
(void)
{
   // Allocate memory for a node array, with 'STACK_INIT_SIZE' slots.
   size_t base_size = sizeof(gstack_t);
   size_t extra_size = STACK_INIT_SIZE * sizeof(void *);
   gstack_t *new = malloc(base_size + extra_size);
   if (new == NULL) {
      fprintf(stderr, "error: could not create gstack\n");
      ERROR = 589;
      return NULL;
   }
   new->nitems = 0;
   new->nslots = STACK_INIT_SIZE;
   return new;
}

gstack_t **
new_tower
(
   int height
)
{
   gstack_t **new = malloc((height+1) * sizeof(gstack_t *));
   if (new == NULL) {
      fprintf(stderr, "error: could not create tower\n");
      ERROR = 606;
      return NULL;
   }
   for (int i = 0 ; i < height ; i++) {
      new[i] = new_gstack();
      if (new[i] == NULL) {
         ERROR = 612;
         fprintf(stderr, "error: could not initialize tower\n");
         while (--i >= 0) {
            free(new[i]);
            new[i] = NULL;
         }
         free(new);
         return NULL;
      }
   }
   new[height] = TOWER_TOP;
   return new;
}


void
destroy_tower
(
   gstack_t **tower
)
{
   for (int i = 0 ; tower[i] != TOWER_TOP ; i++) free(tower[i]);
   free(tower);
}


void
push
(
   void *item,
   gstack_t **stack_addr
)
{
   // Convenience variable for readability.
   gstack_t *stack = *stack_addr;

   // Resize stack if needed.
   if (stack->nitems >= stack->nslots) {
      // If the stack has more items than slots, it is in a
      // locked state and will not receive more items.
      if (stack->nitems > stack->nslots) return;

      // The stack is not locked, allocate more memory.
      int new_nslots = 2 * stack->nslots;
      size_t base_size = sizeof(gstack_t);
      size_t extra_size = new_nslots * sizeof(void *);
      gstack_t *ptr = realloc(stack, base_size + extra_size);
      if (ptr == NULL) {
         // Failed to add item to the stack. Warn and increase
         // 'nitems' beyond 'nslots' to lock the stack.
         fprintf(stderr, "error: could not push to gstack\n");
         stack->nitems++;
         ERROR = 664;
         return;
      }
      *stack_addr = stack = ptr;
      stack->nslots = new_nslots;
   }
   // Push item and increate 'nitems'.
   stack->items[stack->nitems++] = item;
   return;
}


// Snippet to check whether everything went fine.
// If not, ERROR is the line raising the error.
int check_trie_error_and_reset(void) {
   if (ERROR) {
      int last_error_at_line = ERROR;
      ERROR = 0;
      return last_error_at_line;
   }
   return 0;
}

// Snippet to count nodes of a trie.
int count_nodes(trie_t * trie) {
   return recursive_count_nodes(trie->root, get_height(trie), 0);
}

int recursive_count_nodes(node_t * node, int maxdepth, int depth) {
   int count = 1;
   for (int i = 0 ; i < 6 ; i++) {
      if (node->child[i] == NULL) continue;
      node_t *child = (node_t *) node->child[i];
      count += depth < maxdepth - 1 ?
         recursive_count_nodes(child, maxdepth, depth+1) : 1;
   }
   return count;
}
