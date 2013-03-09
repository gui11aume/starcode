#include <string.h>
#include <stdlib.h>
#include "trie.h"

char swap_tmp;
#define SWAP(a,b) swap_tmp = (*a); (*a) = (*b); (*b) = swap_tmp;

char
translate
(
   char c
)
//                                                                        
// SYNOPSIS:                                                              
//   DNA letters are associated to an index between 0 and 3, non DNA      
//   letters have index 4. This saves a lot of code and time during       
//   search and minimizes dynamic memory allocation.                      
//                                                                        
// ARGUMENTS:                                                             
//   'c': the character to translate.                                     
//                                                                        
// RETURN:                                                                
//   The translated character, a number between -1 and 4.                 
//                                                                        
{
   switch(c) {
      case 'A':
         return  0;
      case 'C':
         return  1;
      case 'G':
         return  2;
      case 'T':
         return  3;
      case '\0':
         return -1;
      default:
         return  4;
   }
}


char
untranslate
(
   char c
)
//                                                                        
// SYNOPSIS:                                                              
//   Reverse of the 'translate' function.                                 
//                                                                        
// ARGUMENTS:                                                             
//   'c': the character to untranslate.                                   
//                                                                        
// RETURN:                                                                
//   The untranslated character.                                          
//                                                                        
{
   switch(c) {
      case 0:
         return  'A';
      case 1:
         return  'C';
      case 2:
         return  'G';
      case 3:
         return  'T';
      default:
         return  'N';
   }
}


hitlist *
new_hitlist
(void)
//                                                                        
// SYNOPSIS:                                                              
//   Constructor for a 'hitlist'.                                         
//                                                                        
// RETURN:                                                                
//   An empty 'hitlist'.                                                  
//                                                                        
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
//                                                                        
// SYNOPSIS:                                                              
//   Adds a hit to a hitlist.                                             
//                                                                        
// ARGUMENTS:                                                             
//   'hits': the 'hitlist' to update.                                     
//   'node': the hit to add.                                              
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
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
(
   hitlist *hits
)
//                                                                        
// SYNOPSIS:                                                              
//   Clears a 'hitlist' from all hits, sets the hit number to 0, set      
//   the current position to root and removes the first hit.              
//                                                                        
// ARGUMENTS:                                                             
//   'hits': the 'hitlist' to clear.                                      
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
{
   hits->n_hits = 0;
   hits->node[0] = NULL;
}

void
destroy_hitlist
(
   hitlist *hits
)
//                                                                        
// SYNOPSIS:                                                              
//   Free the memory allocated on a 'hitlist'.                            
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
{
   free(hits->node);
   free(hits);
}


trienode *
new_trie
(void)
//                                                                        
// SYNOPSIS:                                                              
//   Constructor for a trie root.                                         
//                                                                        
// RETURN:                                                                
//   A root node with no parent, no children, no count and no letter.     
//                                                                        
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
//                                                                        
// SYNOPSIS:                                                              
//   Free the memory allocated on a trie.                                 
//                                                                        
// RETURN:                                                                
//   'void'.                                                              
//                                                                        
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
//                                                                        
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
//                                                                        
{
   int i;
   for (i = 0 ; i < strlen(*string) ; i++) {
      char c = translate((*string)[i]);
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
   c = translate(c);
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
      buffer[i] = untranslate(node->c);
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
   char c = translate(*string);
   if (c == -1) {
      // The string is finshed.
      if (node->counter > 0) update_hitlist(hits, node);
      return;
   }
   string++;
   for (int i = 0 ; i < 4 ; i++) {
      int newmaxdist;
      trienode *child;
      if ((newmaxdist = i == c ? maxdist : maxdist-1) < 0) continue;
      if ((child = node->child[i]) == NULL) continue;
      search(child, string, newmaxdist, hits);
   }
   // The 5-th child corresponds to all non DNA letters. It always
   // incurs a mismatch.
   if (node->child[4] != NULL && maxdist > 0)
      search(node->child[4], string, maxdist-1, hits);
}
