#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min3(a,b,c) \
   (((a) < (b)) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

// Macro used for readbility of the code.
#define child_is_a_hit (depth+maxdist >= L) && (child->data != NULL) \
            && (DYNP[L+depth*M] <= maxdist)

// -- Global variables -- //

// Array of characters to keep track of the path in the trie.
char path[MAXBRCDLEN];

// Dynamic programming table. The separate integer pointer initialized
// to 'NULL' allows to test efficiently whether the table has been
// initialized by 'DYNP == NULL'. If true, a call to 'init_search'
// initializes the table content and sets the pointer off 'NULL'.
int *DYNP = NULL;
int _content[M*M];

void init_DYNP(void) {
   memset(_content, 0, M*M * sizeof(int));
   for (int i = 0 ; i < M ; i++) _content[i] = _content[i*M] = i;
   DYNP = _content;
}

// Helper function for the interface.
int hip_error (hip_t *hip) { return hip->error; }

int
cmphit
(
   const void *a,
   const void *b
)
{
   int A = ((hit_t *)a)->dist;
   int B = ((hit_t *)b)->dist;
   return (A > B) - (A < B);
}


hip_t *
new_hip
(void)
// SYNOPSIS:                                                              
//   Constructor for a hip.                                               
//                                                                        
// RETURN:                                                                
//   A point to an empty hip upon success, 'NULL' upon failure.           
{
   hip_t *hip = malloc(sizeof(hip_t));
   if (hip == NULL) return NULL;
   hip->hits = malloc(1024 * sizeof(hit_t));
   if (hip->hits == NULL) {
      free(hip);
      return NULL;
   }
   hip->size = 1024;
   hip->n_hits = 0;
   hip->error = 0;
   return hip;
}

void
add_to_hip
(
   hip_t *hip,
   trienode *node,
   const char *match,
   int dist
)
// SYNOPSIS:                                                              
//   Adds a hit to a hip.                                                 
//                                                                        
// ARGUMENTS:                                                             
//   'hip': the hip to update.                                            
//   'node': the hit node to add.                                         
//   'match': the sequence that matches.                                  
//   'dist': the Levenshtein distance to the query.                       
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   int idx = hip->n_hits;
   // Dynamic growth of the array of hits.
   if (idx >= hip->size) {
      hit_t *ptr = realloc(hip->hits, 2*hip->size * sizeof(hit_t));
      if (ptr != NULL) {
         hip->hits = ptr;
         hip->size *= 2;
      }
      else {
         // TODO: Explain what happened.
         exit(EXIT_FAILURE);
      }
   }
   // Update hip.
   hip->hits[idx].node = node;
   hip->hits[idx].dist = dist;
   strcpy(hip->hits[idx].match, match);
   hip->n_hits++;
}

void
clear_hip
(hip_t *hip)
// SYNOPSIS:                                                              
//   Sets the hit number to 0 and remove the error flag.                  
//                                                                        
// ARGUMENTS:                                                             
//   'hip': the hip to clear.                                             
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   hip->error = 0;
   hip->n_hits = 0;
}


void
destroy_hip
(
   hip_t *hip
)
// SYNOPSIS:                                                              
//   Free the memory allocated on a hip.                                  
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   free(hip->hits);
   free(hip);
}


trienode *
new_trienode
(void)
// SYNOPSIS:                                                              
//   Constructor for a trie node or a  root.                              
//                                                                        
// RETURN:                                                                
//   A root node with no data and no children.                            
{
   trienode *node = malloc(sizeof(trienode));
   if (node == NULL) {
      // Memory error.
      return NULL;
   }
   for (int i = 0 ; i < 5 ; i++) node->child[i] = NULL;
   node->data = NULL;
   return node;
}


void
destroy_nodes_downstream_of
(
   trienode *node,
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


trienode *
find_path
(
   trienode *node,
   const int **translated
)
// SYNOPSIS:                                                              
//   Finds the longest path corresponding to a prefix of 'string' from    
//   a node of the trie.                                                  
//                                                                        
// ARGUMENTS:                                                             
//   'node': the node to start the path from (usually root).              
//   'translated': the address of the translated string to match.         
//                                                                        
// RETURN:                                                                
//   The node where the path ends.                                        
//                                                                        
// SIDE-EFFECTS:                                                          
//   Shifts 'translated' by the length of the path.                       
{

   int c;
   trienode *child;

   while ((c = **translated) != EOS) {
      if ((child = node->child[c]) == NULL) break;
      (*translated)++;
      node = child;
   }

   return node;
}


trienode *
insert
(
   trienode *parent,
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
   trienode *child = new_trienode();
   if (child == NULL) return NULL;
   parent->child[c] = child;
   return child;
}


trienode *
insert_string
(
   trienode *root,
   const char *string
)
// SYNOPSIS:                                                              
//   Helper function to construct tries. Insert a string from a root node 
//   by multiple calls to 'insert_char'.                                  
//                                                                        
// RETURN:                                                                
//   The leaf node in case of succes, 'NULL' otherwise.                   
{
   // Translate the query string.
   int i;
   int translated[M];
   for (i = 0 ; i < strlen(string) ; i++) {
      translated[i] = translate[(int) string[i]];
   }
   translated[i] = EOS;

   const int *toinsert = translated;
   trienode *node = find_path(root, &toinsert);
   for (i = 0 ; toinsert[i] != EOS ; i++) {
      if (node == NULL) return NULL;
      node = insert(node, toinsert[i]);
   }
   return node;
}

void
search_perfect_match
(
   trienode *node,
   int *query,
   hip_t *hip,
   int depth,
   int maxdist
)
{
   int c;
   trienode *child;

   while ((c = *query++) != EOS) {
      if ((c < 0) || (child = node->child[c]) == NULL) return;
      node = child;
      path[depth++] = untranslate[c];
   }

   if (node->data != NULL) {
      path[depth] = '\0';
      add_to_hip(hip, node, path, maxdist);
   }

   return;
}

void
recursive_search
(
   trienode *node,
   int *query,
   int L,
   int maxdist,
   hip_t *hip,
   int depth
)
{

   if (depth > L + maxdist - 1) return;

   depth++;
   trienode *child;

   //int left = max(0, depth-maxdist);
   //int right = depth+maxdist;
   int minj = max(depth - maxdist, 1);
   int maxj = min(depth + maxdist, L);

   // LABEL: "iterate over 5 children".
   for (int i = 0 ; i < 5 ; i++) {

      // Skip if current node has no child with current character.
      if ((child = node->child[i]) == NULL) continue;

      path[depth-1] = untranslate[i];

      int d;
      int mmatch;
      int shift1;
      int shift2;
      int mindist = maxdist + 1;

      // Fill in (part of) the row of index 'depth' of 'DYNP'.
      for (int j = minj ; j <= maxj ; j++) {
         mmatch = DYNP[(j-1)+(depth-1)*M] + (i != query[j-1]);
         shift1 = DYNP[j+(depth-1)*M] + 1;
         shift2 = DYNP[(j-1)+depth*M] + 1;
         d = DYNP[j+depth*M] = min3(mmatch, shift1, shift2);
         if (d < mindist) mindist = d;
      }

      // In case the smallest Levenshtein distance in 'DYNP' is
      // equal to the maximum allowed distance, no more mismatches
      // and indels are allowed. We can shortcut by searching perfect
      // matches.
      
      if (mindist == maxdist) {
         int left = max(depth - maxdist, 0);
         for (int j = left ; j <= maxj ; j++) {
            if (DYNP[j+depth*M] == mindist) {
               search_perfect_match(child, query+j, hip, depth, maxdist);
            }
         }
         continue;
      }

      // We have a hit if the child node is a tail, and the
      // Levenshtein distance is not larger than 'maxdist'. But first
      // we need to make sure that the Levenshtein distance to the
      // query has been computed, which is the first condition of the
      // macro used below.
      // 1. depth+maxdist >= L
      // 2. child->data != NULL
      // 3. DYNP[L+depth*M] <= maxdist
 
      if (child_is_a_hit) {
         path[depth] = '\0';
         add_to_hip(hip, child, path, DYNP[L+depth*M]);
      }

      recursive_search(child, query, L, maxdist, hip, depth);

   }

}

hip_t *
search
(
   trienode *root,
   const char *query,
   int maxdist,
   hip_t *hip
)
// SYNOPSIS:                                                             
//   Wrapper for 'recurse'.                                              
{
   
   if (DYNP == NULL) init_DYNP();

   int L = strlen(query);
   if (L > MAXBRCDLEN) {
      hip->error = 1;
      return hip;
   }

   // Altranslate the query string.
   int i;
   int altranslated[M];
   for (i = 0 ; i < L ; i++) {
      // Non DNA letters or "N" in the query string are translated
      // to -1, so that in the loop labelled "iterate over 5 children"
      // the child of index 0 will always have a mismatch with the
      // query.
      altranslated[i] = altranslate[(int) query[i]];
   }
   altranslated[i] = EOS;

   if (maxdist <= 0) {
      search_perfect_match(root, altranslated, hip, 0, 0);
   }
   else {
      recursive_search(root, altranslated, L, maxdist, hip, 0);
   }

   qsort(hip->hits, hip->n_hits, sizeof(hit_t), cmphit);
   return hip;

}

// Tree representation for debugging.
void
printrie(
   FILE *f,
   trienode *node
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
