#include "trie.h"

#define min(a,b) (((a) < (b)) ? (a):(b))
#define max(a,b) (((a) > (b)) ? (a):(b))
#define min3(a,b,c) \
   (((a) < (b)) ? ((a) < (c) ? (a):(c)):((b) < (c) ? (b):(c)))

// Translations between letters and numbers.
static const char untranslate[5] = "NACGT";
static const char translate[256] = {
   [ 0 ] =-1,
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


// Create a global dynamic programming table.
int dtable[M*M];

hitlist *
new_hitlist
(void)
// SYNOPSIS:                                                              
//   Constructor for a 'hitlist'.                                         
//                                                                        
// RETURN:                                                                
//   An empty 'hitlist'.                                                  
{
   hitlist *hits = malloc(sizeof(hitlist));
   hits->size = 1024;
   hits->n_hits = 0;
   hits->node = malloc(1024 * sizeof(trienode *));
   hits->node[0] = NULL;
   return hits;
}

void
add_to_hitlist
(
   hitlist *hits,
   trienode *node
)
// SYNOPSIS:                                                              
//   Adds a hit to a hitlist.                                             
//                                                                        
// ARGUMENTS:                                                             
//   'hits': the 'hitlist' to update.                                     
//   'node': the hit to add.                                              
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   // Dynamic growth of the array of hits.
   if (hits->n_hits == hits->size) {
      hits->size *= 2;
      trienode **ptr = realloc(hits->node, hits->size);
      if (ptr != NULL) hits->node = ptr;
      else exit(EXIT_FAILURE);
   }
   // Update hits.
   hits->node[hits->n_hits] = node;
   hits->n_hits++;
}


void
clear_hitlist
(hitlist *hits)
// SYNOPSIS:                                                              
//   Clears a 'hitlist' from all hits, sets the hit number to 0, set      
//   the current position to root and removes the first hit.              
//                                                                        
// ARGUMENTS:                                                             
//   'hits': the 'hitlist' to clear.                                      
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   hits->n_hits = 0;
   hits->node[0] = NULL;
}

void
destroy_hitlist
(
   hitlist *hits
)
// SYNOPSIS:                                                              
//   Free the memory allocated on a 'hitlist'.                            
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{
   free(hits->node);
   free(hits);
}


trienode *
new_trie
(void)
// SYNOPSIS:                                                              
//   Constructor for a trie root.                                         
//                                                                        
// RETURN:                                                                
//   A root node with no parent, no children, no count and no letter.     
{
   trienode *root = malloc(sizeof(trienode));
   for (int i = 0 ; i < 5 ; i++) root->child[i] = NULL;
   root->c = -1;
   root->parent = NULL;
   root->counter = -1;
   return root;
}


void
destroy_trie
(
   trienode *root
)
// SYNOPSIS:                                                              
//   Free the memory allocated on a trie.                                 
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
{  
   if (root != NULL) {
      for (int i = 0 ; i < 5 ; i++) destroy_trie(root->child[i]);
      free(root);
   }
}


trienode *
find_path
(
   trienode *node,
   const char **string
)
// SYNOPSIS:                                                              
//   Finds the longest path corresponding to a prefix of 'string' from    
//   a node of the trie.                                                  
//                                                                        
// ARGUMENTS:                                                             
//   'node': the node to start the path from (usually root).              
//   'string': the address of the string to match.                        
//                                                                        
// RETURN:                                                                
//   The node where the path ends.                                        
//                                                                        
// SIDE-EFFECTS:                                                          
//   Shifts 'string' by the length of the path.                           
{
   int i;
   for (i = 0 ; i < strlen(*string) ; i++) {
      char c = translate[(int) (*string)[i]];
      if (node->child[(int) c] == NULL) break;
      node = node->child[(int) c];
   }
   *string += i;
   return node;
}


trienode *
append_to
(
   trienode *parent,
   char c
)
{
   c = translate[(int) c];
   trienode *child = malloc(sizeof(trienode));
   child->c = c;
   for (int i = 0 ; i < 5 ; i++) child->child[i] = NULL;
   child->parent = parent;
   child->counter = 0;
   parent->child[(int) c] = child;
   return child;
}


trienode *
insert
(
   trienode *root,
   const char *string,
   int counter
)
{
   trienode *node = find_path(root, &string);
   for (int i = 0 ; i < strlen(string) ; i++) {
      node = append_to(node, string[i]);
   }
   node->counter += counter;
   return node;
}


char *
seq
(
   const trienode *node,
   char *buffer,
   int buffer_size
)
{
   int i;
   buffer[buffer_size-1] = '\0';
   for (i = buffer_size-2 ; i >= 0; i--) {
      // Stop before root node.
      if (node->parent == NULL) break;
      buffer[i] = untranslate[(int) node->c];
      node = node->parent;
   }
   return buffer + i+1;
}


int
add_to_count
(
   trienode *node,
   int value
)
{
   return node->counter += value;
}

void
recurse
(
   trienode *node,
   char *query,
   int L,
   int maxdist,
   hitlist *hits,
   int table[],
   int depth
)
{

   depth++;
   trienode *child;

   // TODO: treat the case of "N" properly.
   for (int i = 0 ; i < 5 ; i++) {
      if ((child = node->child[i]) == NULL) continue;

      int d;
      int mind;
      int mmatch;
      int shift1;
      int shift2;

      // Fill in (part of) the row of index 'depth' of 'dtable'.
      int minj = max(1, depth-maxdist);
      int maxj = min(depth+maxdist, M-1);

      // Initial case.
      mmatch = dtable[(minj-1)+(depth-1)*M] + (i != query[minj-1]);
      shift1 = dtable[minj+(depth-1)*M] + 1;
      mind = dtable[minj+depth*M] = min(mmatch, shift1);

      for (int j = minj+1 ; j < maxj ; j++) {
         mmatch = dtable[(j-1)+(depth-1)*M] + (i != query[j-1]);
         shift1 = dtable[j+(depth-1)*M] + 1;
         shift2 = dtable[(j-1)+depth*M] + 1;
         d = dtable[j+depth*M] = min3(mmatch, shift1, shift2);
         if (d < mind) mind = d;
      }

      // Final case.
      mmatch = dtable[(maxj-1)+(depth-1)*M] + (i != query[maxj-1]);
      shift2 = dtable[(maxj-1)+depth*M] + 1;
      d = dtable[maxj+depth*M] = min(mmatch, shift2);
      if (d < mind) mind = d;

      // Stop searching if 'maxdist' is exceeded.
      if (mind > maxdist) continue;

      // Check if there is a hit.
      if (
            (depth+maxdist >= L) &&
            (child->counter > 0) &&
            (dtable[L+depth*M] <= maxdist)
         )
      {
         add_to_hitlist(hits, child);
      }

      recurse(child, query, L, maxdist, hits, dtable, depth);

   }
}

void
search
(
   trienode *root,
   char *query,
   int maxdist,
   hitlist *hits
)
// SYNOPSIS:                                                             
//   Wrapper for 'recurse'. Translates the query string and sorts the    
//   hits.                                                               
{

   int i;

   // Translate the query string.
   char translated[MAXBRCDLEN];
   for (i = 0 ; i < strlen(query)+1 ; i++) {
      translated[i] = translate[(int) query[i]];
   }

   for (int i = 0 ; i < maxdist+1 ; i++) dtable[i] = dtable[i*M] = i;

   // Search.
   recurse(root, translated, strlen(query), maxdist, hits, dtable, 0);

}
