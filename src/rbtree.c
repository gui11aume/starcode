/* Two-Dimensional Red-Black Tree.
 * Contains functions for creating, adding and
 * destroying nodes and self-balancing the tree.
 * Querying functions need to be written
 * for each particular implementation.*/

#include "rbtree.h"

const side_t rbinvert[4] = { NE, SE, NW, SW };

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
   rbnode_t * parent = node->parent;
   if (parent != NULL) parent->children[side(node)] = NULL;
   for (int i = 0; i < 4; i++) {
      rbnode_t * child = node->children[i];
      if (child != NULL) destroy_rbnode(child);
   }
   free(node);
}

int
add_child
(
   rbnode_t * child,
   rbnode_t * parent,
   side_t     side
)
{
   // Hang a child on a parent's side.
   if (parent->children[side] != NULL) {
      fprintf(stderr, "error: could not add rbnode\n");
   }
   child->parent = parent;
   parent->children[side] = child;
   // Rebalance the tree if needed.
   if (parent->color == RED) return rebalance(child);
   else return -1;
}

int
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
   for (int i = 0; num_uncles < 3 && i < 4; i++) {
      rbnode_t * sibling = grandpa->children[i];
      if (sibling != NULL && sibling != parent) uncles[num_uncles++] = sibling;
   }
   int num_reds = 0;
   for (int i = 0; i < num_uncles; i++) if (uncles[i]->color == RED) num_reds++;
   if (num_reds) {
      case_propagate(grandpa);
      return 0;
   }
   else { // If all uncles are BLACK.
      int rot = INT_MAX;
      // If node is between grandpa and parent in BOTH dimensions.
      if (side(node) == rbinvert[side(parent)]) {
         node = case_rotate1(node);
         rot = 1;
      }
      case_rotate2(node);
      rot = 2;
      return rot;
   }
}

rbnode_t *
case_rotate1
(
   rbnode_t * node
)
{
   side_t node_side = side(node);
   side_t node_invside = rbinvert[node_side]; 
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
   grandpa->children[side(parent)] = node;
   if (adopted != NULL) adopted->parent = parent;
   parent->children[node_side] = adopted;
   parent->parent = node;
   node->parent = grandpa;
   node->children[node_invside] = parent;
   return parent;
}

void
case_rotate2
(
   rbnode_t * node
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
   side_t parent_side = side(parent);
   side_t parent_invside = rbinvert[parent_side];
   rbnode_t * adopted = parent->children[parent_invside];
   parent->color = BLACK;
   parent->parent = grandpa->parent;
   if (grandpa->parent != NULL) {
      grandpa->parent->children[side(grandpa)] = parent;
   }
   parent->children[parent_invside] = grandpa;
   grandpa->color = RED;
   grandpa->parent = parent;
   grandpa->children[parent_side] = adopted;
   if (adopted != NULL) adopted->parent = grandpa;
}

void
case_propagate
(
   rbnode_t * node
)
{
   // Swap node and children colors and propagate upwards.
   for (int i = 0; i < 4; i++) {
      rbnode_t * child = node->children[i];
      if (child != NULL) child->color = BLACK;
   }
   if (node->parent != NULL) {
      node->color = RED;  
      if (node->parent->color == RED) rebalance(node);
   }
}

side_t
side
(
   rbnode_t * node
)
{
   rbnode_t * parent = node->parent;
   if (parent == NULL) return INT_MAX;
   for (int i = 0; i < 4; i++) {
      if (node == parent->children[i]) return (side_t) i;
   }
   return INT_MAX;
}
