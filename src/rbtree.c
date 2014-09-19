/* Two-Dimensional Red-Black Tree.
 * Contains functions for adding nodes
 * and self-balance the tree.
 */

#include "rbtree.h"

rbnode_t *
new_rbnode
(
   void * item
)
{
   // Allocate and initialize a new rbnode.
   rbnode_t * node = malloc(sizeof(rbnode_t));
   if (node == NULL) {
      fprintf(stderr, "error: could not create rbnode\n");
      ERROR = __LINE__;
      return NULL;
   }
   node->color = RED;
   node->side = NULL;
   node->box = item; // Put the item in the box.
   node->parent = NULL;
   node->children = { NULL, NULL, NULL, NULL };
   return node;
}

void
destroy_node
(
   rbnode_t * node
)
{
   // Free a node and all of its descendants.
   node->parent->children[node->side] = NULL;
   for (int i=0; i<4; i++) {
      rbnode_t * child = node->children[i];
      if (child != NULL) destroy_node(child);
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
   child->side = side;
   child->parent = parent;
   parent->children[side] = child;
   // Rebalance the tree if needed.
   if (parent->color == RED) {
      rebalance(child);
   }
}

void
rebalance
(
   rbnode_t * new
)
{
   // Rebalance the tree according to the case.
   rbnode_t * grandpa = new->parent->parent;
   rbnode_t * parent = new->parent;
   rbnode_t * uncles[3];
   for (int i=0, j=0; j<3 || i<4; i++) {
      rbnode_t * sibling = grandpa->children[i];
      if (sibling->side != parent->side) uncles[j++] = sibling;
   for (int i=0; i<3; i++) {
      if (uncles[i] == NULL || uncles[i]->color == BLACK) {
         if (/* new between parent and granpa */1) {
            /* Rotate like this in case N's value is between G's and P's.
             *
             *      G                G
             *     / \              / \
             *    P   U            N   U
             *   / \       ->     / \
             *  1   N            P   2
             *     / \          / \
             *    A   2        1   A
             *
             * Apply a binary NOT and a 2-bit mask to obtain opposite
             * children index. */
            uint32_t new_invside = invert(new->side); 
            rbnode_t * adopted = new->children[new_invside];
            grandpa->children[parent->side] = new;
            parent->parent = new;
            parent->children[new->side] = adopted;
            new->parent = grandpa;
            new->children[new_invside] = parent;
            new->side = new_invside;
            adopted->parent = parent;
            adopted->side = invert(adopted->side);
            // And change their names according to the new family tree.
            tmp = parent;
            parent = new;
            new = tmp;
         }
         /* Rotate like this in case P's value is between G's and N's.
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
         uint32_t parent_invside = invert(parent->side);
         rbnode_t * adopted = parent->children[parent_invside];
         parent->parent = grandpa->parent;
         parent->children[parent_invside] = grandpa;
         parent->color = BLACK;
         grandpa->parent = parent;
         grandpa->children[parent->side] = adopted;
         grandpa->color = RED;
         tmp = grandpa->side;
         grandpa->side = parent_invside;
         parent->side = tmp;
         // Check whether it is important for all the uncles to be the same color or if it is an implicit property.
         // if one black means all are black, return here. 
      }
   }
   // If all uncles are red also.
   case_all_reds(grandpa);
}

void
case_all_reds
(
   rbnode_t * node
)
{
   // Swap parent/uncles and grandpa colors and propagate upwards.
   for (int i=0; i<4; i++) node->children[i]->color = BLACK;
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
   return (~number) & MASK
}
