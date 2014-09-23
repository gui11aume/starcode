/* Two-Dimensional Red-Black Tree.
 * Contains functions for creating, adding and
 * destroying nodes and self-balancing the tree. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rbtree.h"

char VOID;
double xy[] = { 0.0, 0.0 };

int dfs_shit
(rbnode_t *node)
{
   int num_ndes = 0;
   //int max = 0;
   for (int i = 0 ; i < 4 ; i++){
      if (node->children[i] == NULL) continue;
      num_ndes += dfs_shit(node->children[i]);
      //int thisone = 1 + dfs_shit(node->children[i]); 
      //max = thisone > max ? thisone : max;
   }
   //return max;
   return num_ndes + 1;
}

int main(int argc, const char *argv[])
{
   // Create an example tree.
   rbnode_t * parent = new_rbnode(&VOID, xy);
   rbnode_t * root = parent;
   parent->color = BLACK;
   for (int i=1; i<10000; i++) {
      xy[0] = xy[1] = i;
      rbnode_t * node = new_rbnode(&VOID, xy);
      add_child(node, parent, NE);
      parent = node;
   }
   while (root->parent != NULL) root = root->parent;
   fprintf(stdout, "%d\n", dfs_shit(root));
   return 0;
}

rbnode_t *
new_rbnode
(
   void   * item,
   double * xy // For position sorting.
)
{
   // Allocate and initialize a new RED rbnode.
   rbnode_t * node = calloc(1, sizeof(rbnode_t));
   if (node == NULL) {
      fprintf(stderr, "error: could not create rbnode\n");
      ERROR = __LINE__;
      return NULL;
   }
   memcpy(node->xy, xy, 2*sizeof(double));
   node->box = item; // Put the item in the box.
   return node;
}

void
destroy_rbnode
(
   rbnode_t * node
)
{
   // Free a node and all of its descendants.
   node->parent->children[node->side] = NULL;
   for (int i=0; i<4; i++) {
      rbnode_t * child = node->children[i];
      if (child != NULL) destroy_rbnode(child);
   }
   free(node);
}

void
add_child
(
   rbnode_t * child,
   rbnode_t * parent,
   uint32_t   side
)
{
   // Hang a child on a parent's side.
   if (parent->children[side] != NULL) {
      fprintf(stderr, "error: could not add rbnode\n");
      ERROR = __LINE__;
   }
   child->side = side;
   child->parent = parent;
   parent->children[side] = child;
   // Rebalance the tree if needed.
   if (parent->color == RED) rebalance(child);
}

void
rebalance
(
   rbnode_t * node
)
{
   // Rebalance the tree according to the case.
   rbnode_t * parent = node->parent;
   rbnode_t * grandpa = parent->parent;
   rbnode_t * uncles[3] = { 0 };
   int num_uncles = 0;
   for (int i=0; num_uncles<3 && i<4; i++) {
      rbnode_t * sibling = grandpa->children[i];
      if (sibling != NULL && sibling->side != parent->side) uncles[num_uncles++] = sibling;
   }
   int num_reds = 0;
   for (int i=0; i<num_uncles; i++) {
      if (uncles[i]->color == RED) num_reds++;
   }
   if (num_reds) case_propagate(grandpa);
   else { // If all uncles are BLACK.
      // If node is between grandpa and parent in BOTH dimensions.
      if (node->side == invert(parent->side)) {
         case_rotate1(node);//, parent, grandpa);
         // Change their names according to the new family tree.
         rbnode_t * tmp = parent;
         parent = node;
         node = tmp;
      }
      case_rotate2(node);//, parent, grandpa);
   }
}

void
case_rotate1
(
   rbnode_t * node
   //rbnode_t * parent,
   //rbnode_t * grandpa
)
{
   uint32_t node_invside = invert(node->side); 
   rbnode_t * parent = node->parent;
   rbnode_t * grandpa = parent->parent;
   rbnode_t * adopted = node->children[node_invside];
   /* Rotate like this in case N value is between G and P.
    *
    *      G                G
    *     / \              / \
    *    P   U            N   U
    *   / \       ->     / \
    *  1   N            P   2
    *     / \          / \
    *    A   2        1   A
    */
   if (adopted != NULL) {
      adopted->parent = parent;
      adopted->side = invert(adopted->side);
      parent->children[node->side] = adopted;
   } else parent->children[node->side] = NULL;
   grandpa->children[parent->side] = node;
   parent->parent = node;
   node->parent = grandpa;
   node->children[node_invside] = parent;
   node->side = node_invside;
}

void
case_rotate2
(
   rbnode_t * node
   //rbnode_t * parent,
   //rbnode_t * grandpa
)
{
   /* Rotate like this in case P value is between G and N.
    *
    *        G               P
    *       / \            /   \
    *      P   U          N     G
    *     / \      ->    / \   / \
    *    N   A          1   2 A   U
    *   / \
    *  1   2
    *
    * Obtain a balanced tree. */
   rbnode_t * parent = node->parent;
   rbnode_t * grandpa = parent->parent;
   uint32_t parent_invside = invert(parent->side);
   rbnode_t * adopted = parent->children[parent_invside];
   parent->parent = grandpa->parent;
   parent->children[parent_invside] = grandpa;
   parent->color = BLACK;
   grandpa->parent = parent;
   grandpa->children[parent->side] = adopted;
   grandpa->color = RED;
   uint32_t tmp = grandpa->side;
   grandpa->side = parent_invside;
   parent->side = tmp;
}

void
case_propagate
(
   rbnode_t * node
)
{
   // Swap node and children colors and propagate upwards.
   for (int i=0; i<4; i++) {
      if (node->children[i] != NULL) node->children[i]->color = BLACK;
   }
   if (node->parent != NULL) {
      node->color = RED;  
      if (node->parent->color == RED) rebalance(node);
   }
}

uint32_t
invert
(
   uint32_t number
)
{
   return number ^ MASK;
}
