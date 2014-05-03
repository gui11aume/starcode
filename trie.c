#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// Global error handling.
int ERROR = 0;

// Here functions.
int get_maxtau(node_t *root) { return ((info_t *)root->data)->maxtau; }
int get_height(node_t *root) { return ((info_t *)root->data)->height; }

// --  DECLARATION OF PRIVATE FUNCTIONS  -- //

// Search.
void _search(node_t*, int, struct arg_t);
void dash(node_t*, const int*, struct arg_t);
// Trie creation and destruction.
node_t *insert(node_t*, int, unsigned char);
node_t *new_trienode(unsigned char);
void init_milestones(node_t*);
void destroy_nodes_downstream_of(node_t*, void(*)(void*));
// Utility.
void push(node_t*, narray_t**);
void pushhit(node_t*, hstack_t**, int);


// ------  SEARCH FUNCTIONS ------ //

int
search
(
         node_t   *  trie,
   const char     *  query,
   const int         tau,
         hstack_t ** hits,
   const int         start,
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
      fprintf(stderr, "requested tau greater than 'maxtau'\n");
      return 59;
   }

   int length = strlen(query);
   if (length > height) {
      fprintf(stderr, "query longer than allowed max\n");
      return 65;
   }

   // Make sure the cache is allocated.
   info_t *info = trie->data;

   // Reset the milestones that will be overwritten.
   for (int i = start+1 ; i <= min(trail, height) ; i++) {
      info->milestones[i]->pos = 0;
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
   narray_t *milestones = info->milestones[start];
   for (int i = 0 ; i < milestones->pos ; i++) {
      node_t *start_node = milestones->nodes[i];
      _search(start_node, start + 1, arg);
   }

   // Return the error code of the process (the line of
   // the last error) and 0 if everything went OK.
   return check_trie_error_and_reset();

}


void
_search
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

   // Point to the middle of parent cache, ie the angle of the L.
   // This makes it easier to distinguish the part that goes upward,
   // with positive index and requiring the path, from the part that
   // goes horizontally, with negative index and requiring previous
   // characters of the query.
   char *pcache = node->cache + arg.maxtau + 1;
   // Risk of overflow at depth lower than 'tau'.
   int maxa = min((depth-1), arg.tau);

   // Penalty for match/mismatch and insertion/deletion resepectively.
   unsigned char mmatch;
   unsigned char shift;

   // Part of the cache that is shared between all the children.
   char common[9] = {0,1,2,3,4,5,6,7,8};

   // The branch of the L that is identical among all children
   // is computed separately. It will be copied later.
   uint32_t path = node->path;
   for (int a = maxa ; a > 0 ; a--) {
      // Upper arm of the L (need the path).
      mmatch = pcache[a] + ((path >> 4*(a-1) & 15) != arg.query[depth]);
      shift = min(pcache[a-1], common[a+1]) + 1;
      common[a] = min(mmatch, shift);
   }

   node_t *child;
   for (int i = 0 ; i < 6 ; i++) {
      // Skip if current node has no child at this position.
      if ((child = node->child[i]) == NULL) continue;

      // Same remark as for parent cache.
      char *ccache = child->cache + arg.maxtau + 1;
      memcpy(ccache, common, arg.maxtau * sizeof(char));

      for (int a = maxa ; a > 0 ; a--) {
         // Horizontal arm of the L (need previous characters).
         mmatch = pcache[-a] + (i != arg.query[depth-a]);
         shift = min(pcache[1-a], ccache[-a-1]) + 1;
         ccache[-a] = min(mmatch, shift);
      }
      // Center cell (need both arms to be computed).
      mmatch = pcache[0] + (i != arg.query[depth]);
      shift = min(ccache[-1], ccache[1]) + 1;
      ccache[0] = min(mmatch, shift);

      // Stop searching if 'tau' is exceeded.
      if (ccache[0] > arg.tau) continue;

      // Cache nodes in milestones when trailing.
      if (depth <= arg.trail) push(child, (arg.milestones)+depth);

      // Reached the height, it's a hit!
      if (depth == arg.height) {
         pushhit(child, arg.hits, ccache[0]);
         continue;
      }

      // Use 'dash()' if no more mismatches allowed.
      if ((ccache[0] == arg.tau) && (depth > arg.trail)) {
         dash(child, arg.query+depth+1, arg);
         continue;
      }

      _search(child, depth+1, arg);

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
      if ((c > 4) || (child = node->child[c]) == NULL) return;
      node = child;
   }

   // End of query, check whether node is a tail.
   if (node->data != NULL) pushhit(node, arg.hits, arg.tau);

   return;

}


// ------  TRIE CONSTRUCTION AND DESTRUCTION  ------ //


node_t *
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
      fprintf(stderr, "'maxtau' cannot be greater than 8\n");
      ERROR = 307;
      // DETAIL:                                                         
      // There is an absolute limit at 'tau' = 8 because the struct      
      // member 'path' is encoded as a 32 bit 'int', ie an 8 x 4-bit     
      // array. It should be enough for most practical purposes.         
      return NULL;
   }

   node_t *root = new_trienode(maxtau);
   if (root == NULL) {
      ERROR = 317;
      return NULL;
   }

   info_t *info = malloc(sizeof(info_t));
   if (info == NULL) {
      ERROR = 323;
      free(root);
      return NULL;
   }

   // Set the values of the meta information.
   info->maxtau = maxtau;
   info->height = height;
   memset(info->milestones, 0, M * sizeof(narray_t *));

   root->data = info;
   init_milestones(root);

   if (*(info->milestones) == NULL) {
      ERROR = 336;
      free(info);
      free(root);
      return NULL;
   }

   return root;

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
   size_t cachesize = (2*maxtau + 3) * sizeof(char);
   node_t *node = malloc(sizeof(node_t));
   if (node == NULL) {
      ERROR = 341;
      return NULL;
   }

   node->cache = malloc(cachesize);
   if (node->cache == NULL) {
      free(node);
      ERROR = 348;
      return NULL;
   }

   // Set all base values to 0.
   node->data = NULL;
   node->path = 0;
   for(int i = 0; i < 6; i++) node->child[i] = NULL;
   // Set the cache like a root node.
   for (int i = 0 ; i < 2*maxtau + 3 ; i++) {
      node->cache[i] = (unsigned char) abs(i-1-maxtau);
   }
   return node;
}


node_t *
insert_string
(
         node_t * trie,
   const char   * string
)
// SYNOPSIS:                                                              
//   Front end function to fill in a trie. Insert a string from root, or  
//   simply return the node at the end of the string path if it already   
//   exists.                                                              
//   XXX If the empty string is inserted, this will return the root  XXX  
//   XXX so modifying the 'data' struct member of the resturn value  XXX  
//   XXX without checking can cause terrible bugs.                   XXX  
//                                                                        
// RETURN:                                                                
//   The leaf node in case of succes, 'NULL' otherwise.                   
{

   int i;

   int nchar = strlen(string);
   if (nchar > MAXBRCDLEN) {
      fprintf(stderr, "cannot insert string longer than %d\n", MAXBRCDLEN);
      ERROR = 405;
      return NULL;
   }
   
   unsigned char maxtau = get_maxtau(trie);

   // Find existing path and append one node.
   node_t *node = trie;
   for (i = 0 ; i < nchar ; i++) {
      node_t *child;
      int c = translate[(int) string[i]];
      if ((child = node->child[c]) == NULL) {
         node = insert(node, c, maxtau);
         break;
      }
      node = child;
   }

   // Append more nodes.
   for (i++ ; i < nchar ; i++) {
      if (node == NULL) {
         ERROR = 426;
         return NULL;
      }
      int c = translate[(int) string[i]];
      node = insert(node, c, maxtau);
   }

   return node;

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
      ERROR = 466;
      return NULL;
   }
   // Update child path and parent pointer.
   child->path = (parent->path << 4) + position;
   // Update parent node.
   parent->child[position] = child;

   return child;

}


void
init_milestones
(
   node_t *trie
)
// SYNOPSIS:                                                              
//   Allocate memory for milestones (which is recycled when the trie is   
//   destroyed.)                                                          
//                                                                        
// PARAMETERS:                                                            
//   trie: the trie milestones are associated to                          
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Modifies the 'data' struct member of the trie and allocates memory   
//   accordingly.                                                         
{
   info_t *info = trie->data;
   for (int i = 0 ; i < get_height(trie) + 1 ; i++) {
      info->milestones[i] = new_narray();
      if (info->milestones[i] == NULL) {
         ERROR = 502;
         while (--i >= 0) {
            free(info->milestones[i]);
            info->milestones[i] = NULL;
         }
         return;
      }
   }
   // Push the root into the 0-depth cache.
   // It will be the only node ever in there.
   push(trie, info->milestones);
}


void
destroy_trie
(
   node_t *trie,
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
   info_t *info = trie->data;
   for (int i = 0 ; i < M ; i++) {
      if (info->milestones[i] != NULL) free(info->milestones[i]);
   }
   // Set info to 'NULL' before recursive destruction.
   // This will prevent 'destroy_nodes_downstream_of()' to
   // do double free it.

   free(trie->data);
   trie->data = NULL;

   // ... and bye-bye.
   destroy_nodes_downstream_of(trie, destruct);
}


void
destroy_nodes_downstream_of
(
   node_t *node,
   void (*destruct)(void *)
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
      for (int i = 0 ; i < 6 ; i++) {
         destroy_nodes_downstream_of(node->child[i], destruct);
      }
      if (node->cache != NULL) {
         free(node->cache);
         node->cache = NULL;
      }
      if (node->data != NULL && destruct != NULL) (*destruct)(node->data);
      free(node);
   }
}



// ------  UTILITY FUNCTIONS ------ //


narray_t *
new_narray
(void)
// SYNOPSIS:                                                              
//   Creates a new node array / stack.                                    
//                                                                        
// RETURN:                                                                
//   A pointer to the new node array.                                     
//                                                                        
// SIDE EFFECTS:                                                          
//   Allocates the memory for the node array.                             
{
   // Allocate memory for a node array, with 'STACK_INIT_SIZE' slots.
   narray_t *new = malloc(sizeof(narray_t));
   if (new == NULL) {
      ERROR = 591;
      return NULL;
   }
   new->nodes = (node_t **) malloc(STACK_INIT_SIZE * sizeof(node_t *));
   if (new->nodes == NULL) {
      //fprintf(stderr,"error: new_narray malloc\n");
      free(new);
      ERROR = 597;
      return NULL;
   }
   new->err = 0;
   new->pos = 0;
   new->lim = STACK_INIT_SIZE;
   return new;
}


hstack_t *
new_hstack
(void)
// SYNOPSIS:                                                              
//   Creates a new hit stack.                                             
//                                                                        
// RETURN:                                                                
//   A pointer to the new hit stack.                                      
//                                                                        
// SIDE EFFECTS:                                                          
//   Allocates the memory for the hit stack.                              
{
   hstack_t *new = malloc(sizeof(hstack_t));
   if (new == NULL) {
      ERROR = 640;
      return NULL;
   }
   new->nodes = malloc(STACK_INIT_SIZE * sizeof(node_t *));
   new->dist  = malloc(STACK_INIT_SIZE * sizeof(int));
   if (new->nodes == NULL || new->dist == NULL) {
      //fprintf(stderr,"error: new_hstack malloc\n");
      free(new);
      ERROR = 629;
      return NULL;
   }
   new->err = 0;
   new->pos = 0;
   new->lim = STACK_INIT_SIZE;
   return new;
}


void
destroy_hstack
(
   hstack_t *hstack
)
{
   free(hstack->dist);
   free(hstack->nodes);
   free(hstack);
}

void
push
(
   node_t *node,
   narray_t **stack_addr
)
// SYNOPSIS:                                                              
//   Push a node into a node array.                                       
//                                                                        
// PARAMETERS:                                                            
//   node: the node to push                                               
//   stack_addr: the address of a node array pointer                      
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Updates the node array and can possibly resize it, which is why the  
//   address of the pointer is passed as argument.                        
{
   narray_t *stack = *stack_addr;

   // Resize if needed.
   if (stack->pos >= stack->lim) {
      int newlim = 2 * stack->lim;
      node_t **ptr = realloc(stack->nodes, newlim * sizeof(node_t *));
      if (ptr == NULL) {
         // Cannot add node to stack, increase error number.
         ERROR = 666;
         stack->err++;
         return;
      }
      stack->nodes = ptr;
      stack->lim = newlim;
   }

   stack->nodes[stack->pos++] = node;
}


void
pushhit
(
 node_t    * node,
 hstack_t ** stack_addr,
 int         dist
)
// SYNOPSIS:                                                              
//   Push a node into a hit stack.                                        
//                                                                        
// PARAMETERS:                                                            
//   node: the node to push                                               
//   stack_addr: the address of a hit stack pointer                       
//   dist: the edit distance between the hit and the query                
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
// SIDE EFFECTS:                                                          
//   Updates the hit stack and can possibly resize it, which is why the   
//   address of the pointer is passed as argument.                        
{
   hstack_t * stack = *stack_addr;

   // Resize if needed.
   if (stack->pos >= stack->lim) {
      int newlim = 2 * stack->lim;
      node_t **ptr_1 = realloc(stack->nodes, newlim * sizeof(node_t *));
      int *ptr_2 = realloc(stack->dist, newlim * sizeof(int));
      if (ptr_1 == NULL || ptr_2 == NULL) {
         //fprintf(stderr, "error: push realloc (%s).\n", strerror(errno));
         // Cannot add node to stack, increase error number.
         ERROR = 741;
         stack->err++;
         return;
      }
      stack->nodes = ptr_1;
      stack->dist = ptr_2;
      stack->lim = newlim;
   }
   stack->nodes[stack->pos] = node;
   stack->dist[stack->pos++] = dist;
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
int count_nodes(node_t* node) {
   int count = 1;
   for (int i = 0 ; i < 6 ; i++) {
      if (node->child[i] == NULL) continue;
      count += count_nodes(node->child[i]);
   }
   return count;
}
