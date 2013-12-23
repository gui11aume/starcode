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

void destroy_active_set(node_t *);
nstack_t *new_active_set(node_t *);
void add_to_active_set(node_t *, node_t *);
int dmax(int, node_t *, node_t *);
void add_recursively(int, node_t *, node_t *);
void find_active_nodes(int, node_t *, node_t *);
void pairs(node_t *);
void _triestack(int, node_t **, int, void(*)(node_t *));


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
   node->ans = NULL;
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
   if (node->ans != NULL) free(node->ans);
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
   if (node->ans != NULL) return NULL;

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
   node->ans = active_set;
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
   nstack_t *active_set = (nstack_t *) node->ans;
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
      node->ans = active_set = ptr;
      active_set->lim = newlim;
   }
   active_set->node[active_set->idx++] = active_node;
}


int
dmax
(
   int tau,
   node_t *X,
   node_t *Y
)
{
   // Early exit if string lengths are dissimilar.
   int shift = (X->depth - Y->depth);
   int thresh = tau - abs(shift);
   // Tested before the call.
   //if (thresh < 0) return 0;

   node_t *pX = X;
   node_t *pY = Y;
   // At most one of those is executed.
   for (int i = shift ; i > 0 ; i--) pX = pX->parent;
   for (int i = shift ; i < 0 ; i++) pY = pY->parent;

   // Fill 'DYNP' by "angles".
   int L = 0;
   for ( ; pX != pY ; L++) {

      int mmatch;
      int shift1;
      int shift2;

      // "N" is always a mismatch.
      int seqX[MAXBRCDLEN];
      int seqY[MAXBRCDLEN];
      seqX[L] = X->pos;
      seqY[L] = Y->pos ? Y->pos : 5;

      int maxa = min(L, thresh);
      for (int a = maxa ; a > 0 ; a--) {
         mmatch = DYNP[(L  )+(L-a  )*M] + (seqX[L-a] != seqY[L]);
         shift1 = DYNP[(L+1)+(L-a  )*M] + 1;
         shift2 = DYNP[(L  )+(L-a+1)*M] + 1;
         DYNP[(L+1)+(L-a+1)*M] = min3(mmatch, shift1, shift2);
      }
      for (int a = maxa ; a >= 0 ; a--) {
         mmatch = DYNP[(L-a  )+(L  )*M] + (seqX[L] != seqY[L-a]);
         shift1 = DYNP[(L-a  )+(L+1)*M] + 1;
         shift2 = DYNP[(L-a+1)+(L  )*M] + 1;
         DYNP[(L-a+1)+(L+1)*M] = min3(mmatch, shift1, shift2);
      }

      if (DYNP[(L+1)+(L+1)*M] > thresh) return tau+1;

      X = X->parent; pX = pX->parent;
      Y = Y->parent; pY = pY->parent;

   }

   return DYNP[(L)+(L)*M] + abs(shift);

}

void
add_all_children
(
   node_t *focus,
   node_t *node,
   int depth
)
{
   if (depth == 0) return;
   for (int i = 0 ; i < 5 ; i++) {
      node_t *child = node->child[i];
      if (child == NULL) continue;
      if (child->seen_by != focus) {
         add_to_active_set(focus, child);
         child->seen_by = focus;
      }
      add_all_children(focus, child, depth-1);
   }
}

void
add_recursively
(
   int tau,
   node_t *focus,
   node_t *node
)
{
   int dist;
   if (node->seen_by == focus) return;
   node->seen_by = focus;
   if (abs(focus->depth - node->depth) > tau) return;
   if ((dist = dmax(tau, focus, node)) > tau) return;
   add_to_active_set(focus, node);
   if (focus->depth > node->depth) {
      for (int i = 0 ; i < 5 ; i++) {
         node_t *child = node->child[i];
         if ((child != NULL) && (child->ans != NULL)) {
            add_recursively(tau, focus, child);
         }
      }
   }
   else {
      add_all_children(focus, node, tau-dist);
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
   nstack_t *active_set = parent->ans;

   for (int i = 0 ; i < active_set->idx ; i++) {
      node_t *node = active_set->node[i];
      add_recursively(tau, focus, node);
   }
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



// TRIESTACK

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
      ERROR = 397;
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
triestack
(
   node_t *root,
   int tau,
   void (*callback)(node_t *)
)
{
   
   node_t **stack = malloc(M * sizeof(node_t *));
   if (stack == NULL) {
      ERROR = 429;
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

// -- OLD STUFF

/*
void
perfect_match
(
   node_t *inode,
   node_t *qnode,
   int maxdist,
   int dpth
)
{

   if ((inode->data != NULL) && (qnode->data != NULL)) {
      ipath[dpth] = EOS;
      add_hit(inode, qnode, ipath, maxdist);
   }

   dpth++;

   for (int i = 0 ; i < 5 ; i++) {
      if ((inode->child[i] != NULL) && (qnode->child[i] != NULL)) {
         ipath[dpth-1] = i;
         perfect_match(inode->child[i], qnode->child[i], maxdist, dpth);
      }
   }

   return;

}

void
recursive_search
(
   node_t *inode,
   node_t *qnode,
   int maxdist,
   int dpth
)
{

   if (dpth > MAXBRCDLEN) {
      OK = 0;
      return;
   }

   int maxa = min(maxdist, dpth);
   dpth++;

   for (int i = 0 ; i < 5 ; i++) {

      // Skip if current node has no child with current character.
      if ((istack[dpth] = inode->child[i]) == NULL) continue;
      // Set "N" to 5 instead of 0. This will create a mismatch
      // at the lines labelled MISMATCH.
      ipath[dpth-1] = i == 0 ? 5 : i;

      int d;
      int mmatch;
      int shift1;
      int shift2;
      int minI = maxdist + 1;

      // Fill 'DYNP' horizontally.
      for (int a = maxa ; a > 0 ; a--) {
         // LABEL: MISMATCH
         mmatch = DYNP[(dpth-1)+(dpth-1-a)*M] + (qpath[dpth-a-1] != i);
         shift1 = DYNP[(dpth  )+(dpth-1-a)*M] + 1;
         shift2 = DYNP[(dpth-1)+(dpth  -a)*M] + 1;
         d = DYNP[(dpth)+(dpth-a)*M] = min3(mmatch, shift1, shift2);
         if (d < minI) minI = d;
      }

      if (istack[dpth]->data != NULL) {
         for (int a = maxa ; a > 0 ; a--) {;
            int dist = DYNP[(dpth)+(dpth-a)*M];
            if ((dist <= maxdist) && (qstack[dpth-a]->data != NULL)) {
               ipath[dpth] = EOS;
               add_hit(istack[dpth], qstack[dpth-a], ipath, dist);
            }
         }
      }

      for (int j = 0 ; j < 5 ; j++) {

         if ((qstack[dpth] = qnode->child[j]) == NULL) continue;
         // Set "N" to 5 instead of 0. This will create a mismatch
         // at the lines labelled MISMATCH.
         qpath[dpth-1] = j == 0 ? 5 : j;

         int minQ = maxdist + 1;

         // Fill 'DYNP' vertically.
         for (int a = maxa ; a >= 0 ; a--) {
            // LABEL: MISMATCH
            mmatch = DYNP[(dpth-1-a)+(dpth-1)*M] + (ipath[dpth-a-1] != j);
            shift1 = DYNP[(dpth-1-a)+(dpth  )*M] + 1;
            shift2 = DYNP[(dpth  -a)+(dpth-1)*M] + 1;
            d = DYNP[(dpth-a)+(dpth)*M] = min3(mmatch, shift1, shift2);
            if (d < minQ) minQ = d;
         }

         // In case the smallest Levenshtein distance in 'DYNP' is
         // equal to the maximum allowed distance, no more mismatches
         // and indels are allowed. We can shortcut by searching perfect
         // matches.

         if (min(minI, minQ) == maxdist) {
            for (int a = maxa + (dpth == 1); a > 0 ; a--) {
               if (DYNP[(dpth)+(dpth-a)*M] == maxdist) {
                  perfect_match(istack[dpth], qstack[dpth-a],
                        maxdist, dpth);
               }
            }
            for (int a = maxa + (dpth == 1); a >= 0 ; a--) {
               if (DYNP[(dpth-a)+(dpth)*M] == maxdist) {
                  perfect_match(istack[dpth-a], qstack[dpth],
                        maxdist, dpth-a);
               }
            }
            continue;
         }

         if (qstack[dpth]->data != NULL) {
            for (int a = maxa ; a >= 0 ; a--) {;
               int dist = DYNP[(dpth-a)+(dpth)*M];
               if ((dist <= maxdist) && (istack[dpth-a]->data != NULL)) {
                  int orig = ipath[dpth-a];
                  ipath[dpth-a] = EOS;
                  add_hit(istack[dpth-a], qstack[dpth], ipath, dist);
                  ipath[dpth-a] = orig;
               }
            }
         }

         recursive_search(istack[dpth], qstack[dpth], maxdist, dpth);

      }

   }

}


mset_t *
search
(
   node_t *iroot,
   char *query,
   int maxdist
)
// SYNOPSIS:                                                             
//   Wrapper for 'recurse'.                                              
{
   
   if (DYNP == NULL) init_DYNP();

   node_t *qroot = new_trienode();
   if (qroot == NULL) {
      OK = 0;
      return NULL;
   }
   node_t *node = insert_string(qroot, query);
   if (node == NULL) {
      OK = 0;
      return NULL;
   }
   node->data = node;

   istack[0] = iroot;
   qstack[0] = qroot;

   recursive_search(iroot, qroot, maxdist, 0);

   // Detach the match set from the trie...
   mset_t *mset = node->mset;
   node->mset = NULL;
   // ... and destroy the trie.
   destroy_nodes_downstream_of(qroot, NULL);

   qsort(mset->matches, mset->idx, sizeof(match_t), cmphit);
   return mset;


}

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
*/
