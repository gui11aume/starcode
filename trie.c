#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min3(a,b,c) \
   (((a) < (b)) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))


int ERROR = 0;     // Indicate error at given line of code.

// Dynamic programming table. The separate integer pointer initialized
// to 'NULL' allows to test efficiently whether the table has been
// initialized by 'DYNP == NULL'. If true, a call to 'init_search'
// initializes the table content and sets the pointer off 'NULL'.
int *DYNP = NULL;
int DYNP_content[M*M];

int PATH1[2*M];
int PATH2[2*M];

nstack_t *new_active_set(node_t *);
void destroy_active_set(node_t *);
void add_to_active_set(node_t *, node_t *);
void top_down_search(int, node_t *, node_t *);
void find_active_nodes(int, node_t *, node_t *);
void pairs(node_t *);
void _triestack(int, node_t **, int, void(*)(node_t *));
void add_all_children(node_t *, node_t *, int);
void clear_visits(node_t *);
void init_DYNP(void);
void walk_down(node_t *, node_t *, int, int *, int *, int);
int ondepth(const void *, const void*);
int fill_DYNP_angle(int, int, int *, int *);


void
triestack
(
   node_t *root,
   int tau,
   void (*callback)(node_t *)
)
{
   
   node_t **stack = malloc(M * sizeof(node_t *));
   if (stack == NULL) {
      ERROR = 47;
      return;
   }

   // Initialize.
   init_DYNP();
   stack[0] = root;
   new_active_set(root);

   // Trie-(path)-stack
   _triestack(tau, stack, 1, callback);

   // Clean.
   destroy_active_set(root);
   clear_visits(root);
   free(stack);

}


void
_triestack
(
   int tau,
   node_t **stack,
   int depth,
   void (*callback)(node_t *)
)
{

   if (depth > MAXBRCDLEN) {
      ERROR = 74;
      return;
   }

   node_t *parent = stack[depth-1];
   for (int i = 0 ; i < 5 ; i++) {
      node_t *child = parent->child[i];
      if (child == NULL) continue;
      new_active_set(child);
      find_active_nodes(tau, child, parent);
      for (int a = 0 ; a < min(depth, tau) ; a++) {
         add_to_active_set(stack[depth-1-a], child);
      }
      stack[depth] = child;
      _triestack(tau, stack, depth+1, callback);
      (*callback)(child);
      destroy_active_set(child);
   }
}


void
find_active_nodes
(
   int tau,
   node_t *focus,
   node_t *parent
)
// SYNOPSIS:                                                              
//   Prototype.                                                           
{
   nstack_t *active_set = parent->as;

   //qsort(active_set->node, active_set->idx, sizeof(node_t *), ondepth);
   for (int i = 0 ; i < active_set->idx ; i++) {
      node_t *node = active_set->node[i];
      if (node->seen_by == focus) continue;
      top_down_search(tau, focus, node);
   }
}


void
see_children
(
   int depth,
   node_t *focus,
   node_t *node
)
{
   for (int i = 0 ; i < 5 ; i++) {
      node_t *child = node->child[i];
      if (child == NULL) continue;
      child->seen_by = focus;
      if (depth > 0) see_children(depth-1, focus, child);
   }
}


void
top_down_search
(
   int tau,
   node_t *focus,
   node_t *node
)
{

   node->seen_by = focus;

   int L;
   int lim;
   int mmatch;
   int shift1;
   int shift2;
   int *fpath = PATH1 + M;
   int *npath = PATH2 + M;

   // Find common ancestor.
   const int off = (node->depth - focus->depth);

   node_t *pf = focus;
   node_t *pn = node;

   // At most one of those is executed.
   for (int i = off ; i < 0 ; i++) {
      // "N" is always a mismatch.
      *--fpath = pf->pos ? pf->pos : 5;
      pf = pf->parent;
   }
   for (int i = off ; i > 0 ; i--) {
      *--npath = pn->pos;
      pn = pn->parent;
   }

   // Go backward until we hit the common ancestor.
   for (lim = 0 ; pf != pn ; lim++) {
      // "N" is always a mismatch.
      *--fpath = pf->pos ? pf->pos : 5;
      *--npath = pn->pos;
      pf = pf->parent;
      pn = pn->parent;
   }

   // Fill the stem 'DYNP' depending on the offset.
   if (off < 0) {
      // Fill horizontally.
      int mina = max(-tau,off);
      for (L = 0 ; L < lim ; L++) {
         int maxa = min(L, tau);
         int mindist = INT_MAX;
         for (int a = maxa ; a >= mina ; a--) {
            mmatch = DYNP[(L  )+(L-a  )*M] + (fpath[L-a] != npath[L]);
            shift1 = DYNP[(L+1)+(L-a  )*M] + 1;
            shift2 = DYNP[(L  )+(L-a+1)*M] + 1;
            DYNP[(L+1)+(L-a+1)*M] = min3(mmatch, shift1, shift2);
            if (DYNP[(L+1)+(L-a+1)*M] < mindist) {
               mindist = DYNP[(L+1)+(L-a+1)*M];
            }
         }
         if (mindist > tau) {
            see_children(tau-off, focus, node);
            return;
         }
      }
   }
   else {
      // Fill by angles.
      for (L = 0 ; L < lim ; L++) {
         int maxa = min(L, tau);
         int mindist = INT_MAX;
         for (int a = maxa ; a > 0 ; a--) {
            mmatch = DYNP[(L-a  )+(L  )*M] + (fpath[L] != npath[L-a]);
            shift1 = DYNP[(L-a  )+(L+1)*M] + 1;
            shift2 = DYNP[(L-a+1)+(L  )*M] + 1;
            DYNP[(L-a+1)+(L+1)*M] = min3(mmatch, shift1, shift2);
            if (DYNP[(L-a+1)+(L+1)*M] < mindist) {
               mindist = DYNP[(L-a+1)+(L+1)*M];
            }
         }
         for (int a = maxa ; a >= 0 ; a--) {
            mmatch = DYNP[(L  )+(L-a  )*M] + (fpath[L-a] != npath[L]);
            shift1 = DYNP[(L+1)+(L-a  )*M] + 1;
            shift2 = DYNP[(L  )+(L-a+1)*M] + 1;
            DYNP[(L+1)+(L-a+1)*M] = min3(mmatch, shift1, shift2);
            if (DYNP[(L+1)+(L-a+1)*M]) {
               mindist = DYNP[(L+1)+(L-a+1)*M];
            }
         }
         if (mindist > tau) {
            see_children(tau-off, focus, node);
            return;
         }
      }
      // Add horizontally.
      for ( ; L < lim + off ; L++) {
         int maxa = min(L, tau);
         int mindist = INT_MAX;
         for (int a = maxa ; a >= off ; a--) {
            mmatch = DYNP[(L  )+(L-a  )*M] + (fpath[L-a] != npath[L]);
            shift1 = DYNP[(L+1)+(L-a  )*M] + 1;
            shift2 = DYNP[(L  )+(L-a+1)*M] + 1;
            DYNP[(L+1)+(L-a+1)*M] = min3(mmatch, shift1, shift2);
            if (DYNP[(L+1)+(L-a+1)*M]) {
               mindist = DYNP[(L+1)+(L-a+1)*M];
            }
         }
         if (mindist > tau) {
            see_children(tau-off, focus, node);
            return;
         }
      }
   }
   
   if (DYNP[(L)+(L-off)*M] <= tau) {
      add_to_active_set(focus, node);
   }

   for (int i = 0 ; i < 5 ; i++) {
      node_t *child = node->child[i];
      if (child == NULL || child->as == NULL) continue;
      walk_down(focus, child, L, fpath, npath, tau);
   }

}


void
walk_down
(
   node_t *focus,
   node_t *node,
   int L,
   int *fpath,
   int *npath,
   int tau
)
{

   node->seen_by = focus;

   const int off = (node->depth - focus->depth);

   npath[L] = node->pos;

   int mmatch;
   int shift1;
   int shift2;

   int maxa = min(L, tau);
   int mindist = INT_MAX;
   for (int a = maxa ; a >= off ; a--) {
      mmatch = DYNP[(L  )+(L-a  )*M] + (fpath[L-a] != npath[L]);
      shift1 = DYNP[(L+1)+(L-a  )*M] + 1;
      shift2 = DYNP[(L  )+(L-a+1)*M] + 1;
      DYNP[(L+1)+(L-a+1)*M] = min3(mmatch, shift1, shift2);
      if (DYNP[(L+1)+(L-a+1)*M] < mindist) {
         mindist = DYNP[(L+1)+(L-a+1)*M];
      }
   }

   if (mindist > tau) {
      see_children(tau-off, focus, node);
      return;
   }
   if (DYNP[(L+1)+(L+1-off)*M] <= tau) {
      add_to_active_set(focus, node);
   }

   for (int i = 0 ; i < 5 ; i++) {
      node_t *child = node->child[i];
      if (child == NULL || child->as == NULL) continue;
      walk_down(focus, child, L+1, fpath, npath, tau);
   }

}


int check_trie_error_and_reset(void) {
   if (ERROR) {
      int the_error_is_at_line = ERROR;
      ERROR = 0;
      return the_error_is_at_line;
   }
   return 0;
}


void init_DYNP(void)
{
   for (int i = 0 ; i < M ; i++) {
   for (int j = 0 ; j < M ; j++) {
      DYNP_content[i+j*M] = abs(i-j);
   }
   }
   DYNP = DYNP_content;
}


// -- Trie and node functions -- //

void
clear_visits
(
   node_t *node
)
{
   for (int i = 0 ; i < 5 ; i++) {
      if (node->child[i] != NULL) clear_visits(node->child[i]);
   }
   node->seen_by = NULL;
   node->as = NULL;
}


node_t *
new_trienode(void)
// SYNOPSIS:                                                              
//   Constructor for a trie node or a  root with all pointers set to      
//   'NULL'.                                                              
//                                                                        
// RETURN:                                                                
//   A (root) node with no data and no children upon sucess, 'NULL' upon  
//   failure.                                                             
{
   node_t *node = malloc(sizeof(node_t));
   if (node == NULL) {
      ERROR = 77;
      return NULL;
   }
   memset(node, 0, sizeof(node_t));
   return node;
}


void
destroy_active_set
(
   node_t *node
)
// SYNOPSIS:                                                              
//   Prototype.                                                           
//   We free but do not set to NULL. This allows to know whether a node
//   was visited.
{
   if (node->as != NULL) free(node->as);
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
      destroy_active_set(node);
      free(node);
   }
}


node_t *
insert
(
   node_t *parent,
   int c
)
// SYNOPSIS:                                                              
//   Helper function to construct tries. Append a child to an existing    
//   node at a position specified by the character 'c'. No checking is    
//   performed to make sure that this does not overwrite an existings     
//   node child (causing a memory leak). Since this function is called    
//   exclusiverly by 'insert_string' after a call to 'find_path', this    
//   checking is not required. If 'insert' is called in another context,  
//   this check has to be performed.                                      
//                                                                        
// RETURN:                                                                
//   The appended child node in case of success, 'NULL' otherwise.        
{
   // Initilalize child node.
   node_t *child = new_trienode();
   if (child == NULL) {
      ERROR = 143;
      return NULL;
   }
   child->pos = c;
   child->depth = parent->depth + 1;
   child->parent = parent;
   // Update parent node.
   parent->child[c] = child;
   return child;
}


node_t *
insert_string
(
   node_t *node,
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
      ERROR = 170;
      return NULL;
   }
   
   int i;

   // Find existing path and append one node.
   for (i = 0 ; i < nchar ; i++) {
      node_t *child;
      int c = translate[(int) string[i]];
      if ((child = node->child[c]) == NULL) {
         node = insert(node, c);
         break;
      }
      node = child;
   }

   // Append more nodes.
   for (i++ ; i < nchar ; i++) {
      if (node == NULL) {
         ERROR = 190;
         return NULL;
      }
      int c = translate[(int) string[i]];
      node = insert(node, c);
   }

   return node;

}


// -- Active set functions -- //

nstack_t *
new_active_set
(
   node_t *node
)
// SYNOPSIS:                                                              
//   Prototype.                                                           
{
   // Do not erase anything.
   if (node->as != NULL) return NULL;

   // Allocate memory.
   size_t totalsize = 2*sizeof(int) + 512*sizeof(node_t *);
   nstack_t *active_set = malloc(totalsize);
   if (active_set == NULL) {
      ERROR = 219;
      return NULL;
   }

   // Set initial values.
   memset(active_set, 0, totalsize);
   active_set->lim = 512;

   // Append to 'node' and return;
   node->as = active_set;
   return active_set;

}


void
add_to_active_set
(
   node_t *node,
   node_t *active_node
)
// SYNOPSIS:                                                              
//   Prototype.                                                           
{
   nstack_t *active_set = (nstack_t *) node->as;
   // Allocate more space if stack is full.
   if (active_set->idx >= active_set->lim) {
      int newlim = 2*active_set->lim;
      //size_t oldsize = 2*sizeof(int) + active_set->lim * sizeof(node_t *);
      size_t newsize = 2*sizeof(int) + newlim * sizeof(node_t *);
      //nstack_t *ptr = malloc(newsize);
      nstack_t *ptr = realloc(active_set, newsize);
      if (ptr == NULL) {
         ERROR = 252;
         return;
      }
      //memcpy(ptr, active_set, oldsize);
      //free(active_set);
      node->as = active_set = ptr;
      active_set->lim = newlim;
   }
   active_set->node[active_set->idx++] = active_node;
}


int
ondepth
(
   const void *node_a,
   const void *node_b
)
{
   int a = (*(node_t **)node_a)->depth;
   int b = (*(node_t **)node_b)->depth;
   return (a > b) - (a < b);
}


/*
char *
untranslate_path
(
   char *to,
   int *path
)
{
   int i;
   for (i = 0 ; i < MAXBRCDLEN ; i++) {
      if (path[i] == EOS) break;
      to[i] = untranslate[path[i]];
   }
   to[i] = '\0';
   return to;
}
*/



// Tree representation for debugging.
void
printrie(
   FILE *f,
   node_t *node
)
{
   for (int i = 0 ; i < 5 ; i++) {
      if (node->child[i] != NULL) {
         if (node->child[i]->data != NULL) {
            fprintf(f, "(%c)", untranslate[i]);
         }
         else {
            fprintf(f, "%c", untranslate[i]);
         }
         printrie(f, node->child[i]);
      }
   }
   fprintf(f, "*");
}

void
printDYNP(
   FILE *f,
   int outto
)
{
   for (int i = 0 ; i < outto ; i++) {
      fprintf(f, "%02d", DYNP[0+i*M]);
      for (int j = 0 ; j < outto ; j++) {
         fprintf(f, ",%02d", DYNP[j+i*M]);
      }
      fprintf(f, "\n");
   }
}
