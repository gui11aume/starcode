#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min3(a,b,c) \
   (((a) < (b)) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

// Macro used for readbility of the code.
#define child_is_a_hit (depth+tau >= limit) && (child->data != NULL) \
            && (DYNP[depth+limit*M] <= tau)

// Module error reporting.
int ERROR;

// Array of nodes for the hits.
narray_t *HITS = NULL;

// Dynamic programming table.
int *DYNP = NULL;
int DYNP_CONTENT[M*M];



// Local functions.
node_t *insert(node_t*, int);
void recursive_search(node_t*, int*, int, int, int);
void dash(node_t*, int*);
void addhit(node_t*);
void init_DYNP(void);
int check_trie_error_and_reset(void);


// ------  SEARCH  ------ //


narray_t *
search
(
   node_t *root,
   const char *query,
   int tau,
   narray_t *hits
)
{
   
   if (DYNP == NULL) init_DYNP();
   HITS = hits;

   int length = strlen(query);
   if (length > MAXBRCDLEN) {
      ERROR = 144;
      return NULL;
   }

   // Translate the query string.
   int altranslated[M];
   for (int i = 0 ; i < length ; i++) {
      // Non DNA letters or "N" in the query string are translated
      // to 5, so that in the loop labelled "iterate over 5 children"
      // "N" is always a mismatch.
      altranslated[i] = altranslate[(int) query[i]];
   }
   altranslated[length] = EOS;

   if (tau > 0) {
      recursive_search(root, altranslated, tau, 1, length);
   }
   else {
      dash(root, altranslated);
   }

   HITS = NULL;
   return hits;

}


void
recursive_search
(
   node_t *node,
   int *query,
   int tau,
   int depth,
   int limit
)
{

   if (depth > limit + tau) return;

   node_t *child;

   int mina = max(depth-tau, 1);
   int maxa = min(depth+tau, limit) + 1;

   // LABEL: "iterate over 5 children".
   for (int i = 0 ; i < 5 ; i++) {

      // Skip if current node has no child with current character.
      if ((child = node->child[i]) == NULL) continue;
      int mindist = INT_MAX;

      // Fill in (part of) the row of index 'depth' of 'DYNP'.
      for (int a = mina ; a < maxa ; a++) {
         int mmatch = DYNP[(depth-1)+(a-1)*M] + (i != query[a-1]);
         int shift1 = DYNP[(depth-1)+(a  )*M] + 1;
         int shift2 = DYNP[(depth  )+(a-1)*M] + 1;
         DYNP[(depth)+(a)*M] = min3(mmatch, shift1, shift2);
         if (DYNP[(depth)+(a)*M] < mindist) mindist = DYNP[(depth)+(a)*M];
      }

      // Early exit if tau is exceeded.
      if (mindist > tau) return;

      // In case the smallest Levenshtein distance in 'DYNP' is
      // equal to the maximum allowed distance, no more mismatches
      // and indels are allowed. We can shortcut by searching perfect
      // matches.
      if (mindist == tau) {
         int left = max(depth-tau, 0);
         for (int a = left ; a < maxa ; a++) {
            if (DYNP[(depth)+(a)*M] == mindist) {
               dash(child, query+a);
            }
         }
         continue;
      }

      // We have a hit if the child node is a tail, and the
      // Levenshtein distance is not larger than 'tau'. But first
      // we need to make sure that the Levenshtein distance to the
      // query has been computed, which is the first condition of the
      // macro used below.
      // 1. depth+tau >= limit
      // 2. child->data != NULL
      // 3. DYNP[depth+limit*M] <= tau
      if (child_is_a_hit) {
         addhit(child);
      }

      recursive_search(child, query, tau, depth+1, limit);

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
      addhit(node);
   }
   return;
}


void
addhit
(
   node_t *node
)
{
   if (HITS->idx >= HITS->lim) {
      size_t newsize = 2 * sizeof(int) + 2*HITS->lim * sizeof(node_t *);
      narray_t *ptr = realloc(HITS, newsize);
      if (ptr == NULL) {
         ERROR = 165;
         return;
      }
      HITS = ptr;
      HITS->lim *= 2;
   }
   HITS->nodes[HITS->idx++] = node;
}


// ------  TRIE CONSTRUCTION  ------ //


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
      ERROR = 68;
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
      ERROR = 44;
      return NULL;
   }
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
      ERROR = 261;
      return NULL;
   }
   new->idx = 0;
   new->lim = 32;
   return new;
}


node_t *
new_trienode
(void)
// SYNOPSIS:                                                              
//   Constructor for a trie node or a  root.                              
//                                                                        
// RETURN:                                                                
//   A root node with no data and no children.                            
{
   node_t *node = malloc(sizeof(node_t));
   if (node == NULL) {
      ERROR = 301;
      return NULL;
   }
   for (int i = 0 ; i < 5 ; i++) node->child[i] = NULL;
   node->data = NULL;
   return node;
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


void init_DYNP(void) {
   // The separate integer pointer initialized to 'NULL' allows to
   // test efficiently whether the table has been initialized by
   // 'DYNP == NULL'. If true, a call to 'init_search' initializes
   // the table content and sets the pointer off 'NULL'.
   for (int i = 0 ; i < M ; i++) {
   for (int j = 0 ; j < M ; j++) {
      DYNP_CONTENT[i+j*M] = abs(i-j);
   }
   }
   DYNP = DYNP_CONTENT;
}


int check_trie_error_and_reset(void) {
   if (ERROR) {
      int the_error_is_at_line = ERROR;
      ERROR = 0;
      return the_error_is_at_line;
   }
   return 0;
}
