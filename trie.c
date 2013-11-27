#include <string.h>
#include <stdlib.h>
#include "trie.h"

// Translations between letters and numbers.
static const char untranslate[5] = "NACGT";
static const char translate[256] = {
   [ 0 ] =-1,
   ['a'] = 1, ['c'] = 2, ['g'] = 3, ['t'] = 4,
   ['A'] = 1, ['C'] = 2, ['G'] = 3, ['T'] = 4,
};


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
update_hitlist
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
search
(
   trienode *node,
   char *string,
   int maxdist,
   hitlist *hits
)
{
   char c = translate[(int) *string];
   if (c == -1) {
      // The string is finished.
      if (node->counter > 0) update_hitlist(hits, node);
      return;
   }
   string++;
   // XXX More than 80% of the time is spent on the bit of code below.
   for (int i = 1 ; i < 5 ; i++) {
      int newmaxdist;
      trienode *child;
      if ((newmaxdist = i == c ? maxdist : maxdist-1) < 0) continue;
      if ((child = node->child[i]) == NULL) continue;
      search(child, string, newmaxdist, hits);
   }
   // The 5-th child corresponds to all non DNA letters. It always
   // incurs a mismatch.
   if (node->child[0] != NULL && maxdist > 0)
      search(node->child[0], string, maxdist-1, hits);
}
