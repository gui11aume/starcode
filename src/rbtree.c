/* Two-Dimensional Red-Black Tree.
 * Contains functions for adding nodes
 * and self-balance the tree. */

#include "rbtree.h"

rbnode_t *
add_child
(
   rbnode_t * parent,
   void     * item,
   int        side
)
{
   rbnode_t * child = malloc(sizeof(rbnode_t));
   parent->children[side] = child;
   child->color = RED;
   child->side = side;
   child->box = item;
   child->parent = parent;
   child->children[IB] = NULL;
   child->children[IA] = NULL;
   child->children[OB] = NULL;
   child->children[OA] = NULL;
   if (parent->color == RED) {
      rebalance(child);
   }
   return child;
}

void
rebalance
(
   rbnode_t * node
)
{
   rbnode_t * grandpa = node->parent->parent;
   rbnode_t * parent = node->parent;
   rbnode_t * uncles[3];
   for (int i=0, j=0; j<3 || i<4; i++) {
      rbnode_t * sibling = grandpa->children[i];
      if (sibling->side != parent->side) uncles[j++] = sibling;
   for (int i=0; i<3; i++) {
      if (uncles[i] == NULL || uncles[i]->color == BLACK) {
         if (/* node between parent and granpa */1) {
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
            rbnode_t * adopted = node->children[(~node->side) & mask];
            grandpa->children[parent->side] = node;
            parent->parent = node;
            parent->children[node->side] = adopted;
            node->parent = grandpa;
            node->children[(~node->side) & mask] = parent;
            node->side = (~node->side) & mask;
            adopted->parent = parent;
            adopted->side = (~adopted->side) & mask;
            // And change their names according to the new family tree.
            tmp = parent;
            parent = node;
            node = tmp;
         }
         /* Rotate like this in case P's value is between G's and N's.
          *
          *        G               P
          *       / \            /   \
          *      P   U          N     G
          *     / \      ->    / \   / \
          *    N   3          1   2 3   U
          *   / \
          *  1   2
          *
          * Obtain a balanced tree. */
         parent->parent = grandpa->parent;
         parent->color = BLACK;
         parent->children[(~parent->side) & mask];
         grandpa->parent = parent;
         grandpa->children[parent->side] = parent->children[(~parent->side) & mask];
      }
   }
   // If all uncles are red, swap uncles and parent colors and propagate.
   parent->color = BLACK;
   for (int i=0; i<3; i++) uncles[i]->color = BLACK;
   if (grandpa->parent != NULL) {
      grandpa->color = RED;
      if (grandpa->parent->color == RED) rebalance(grandpa);
   }
}
