#include "example.h"
#include "unittest.h"
#include "rbtree.h"

double VOID1[2] = { 0.0 };
double VOID2[2] = { 0.0 };
double VOID3[2] = { 0.0 };
double VOID4[2] = { 0.0 };
side_t side = 0; 

void
test_node_creation_destruction
(void)
{
   for (int i = 0; i < 50; i++) {
      rbnode_t * node = new_rbnode(&VOID1, VOID1);
      test_assert(node != NULL);
      test_assert(node->color == NULL);
      test_assert(node->color == RED);
      test_assert(node->xy == VOID);
      test_assert(node->box == &VOID);
      test_assert(node->parent == NULL);
      for (int i = 0; i < 4; i++) {
         test_assert(node->children[i] == NULL);
      }
      destroy_rbnode(node);
      node = NULL;
   }
}

void
test_node_destruction
(void)
{
   rbnode_t * parent = new_rbnode(&VOID1, VOID1);
   rbnode_t * node = new_rbnode(&VOID2, VOID2);
   add_child(node, parent, side);
   destroy_rbnode(node);
   test_assert(parent->children[side] == NULL);
   destroy_rbnode(parent);
}

void
test_child_addition
(void)
{
   rbnode_t * parent = new_rbnode(&VOID1, VOID1);
   parent->color = BLACK;
   rbnode_t * node = new_rbnode(&VOID2, VOID2);
   add_child(node, parent, side);
   test_assert(parent->children[side] == node);
   test_assert(node->parent == parent);
   destroy_rbnode(parent);
}

void
test_rebalancing_propagate
(void)
{
   // Create example tree.
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   rbnode_t * grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   // Test not rebalancing.
   test_assert(add_child(grandpa, root, 0) == -1);
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
   }
   rbnode_t * node = new_rbnode(&VOID4, VOID4);
   // Test case rebalance-propagate (all siblings red).
   test_assert(add_child(node, grandpa->children[0], 0) == 0);
   // Destroy example tree and create it anew.
   destroy_rbnode(grandpa);
   grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   // Test not rebalancing.
   test_assert(add_child(grandpa, root, 0) == -1);
   for (int i = 0; i < 3; i++) {
      sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
   }
   grandpa->children[2]->color = BLACK;
   node = new_rbnode(&VOID4, VOID4);
   // Test case rebalance-propagate (1 sibling red, 1 black, 1 leaf).
   test_assert(add_child(node, grandpa->children[0], 0) == 0);
   destroy_rbnode(root);
}

void
test_propagate
(void)
{
   // Create example tree.
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   rbnode_t * grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   add_child(grandpa, root, 0);
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
   }
   rbnode_t * node = new_rbnode(&VOID4, VOID4);
   add_child(node, grandpa->children[0], 0);
   // Test case propagate (all siblings red).
   test_assert(root->color == BLACK);
   test_assert(grandpa->color == RED);
   for (int i = 0; i < 4; i++) {
      test_assert(grandpa->children[i]->color == BLACK);
   }
   test_assert(node->color == RED);
   // Destroy example tree and create it anew.
   destroy_rbnode(grandpa);
   grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   add_child(grandpa, root, 0);
   for (int i = 0; i < 3; i++) {
      sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
   }
   grandpa->children[2]->color = BLACK;
   node = new_rbnode(&VOID4, VOID4);
   add_child(node, grandpa->children[0], 0);
   // Test case propagate (1 sibling red, 1 black, 1 leaf).
   test_assert(root->color == BLACK);
   test_assert(grandpa->color == RED);
   for (int i = 0; i < 4; i++) {
      if (grandpa->children[i] == NULL) continue;
      test_assert(grandpa->children[i]->color == BLACK);
   }
   test_assert(node->color == RED);
   destroy_rbnode(root);
}

void
test_rebalancing_rotate1
(void)
{
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID2, VOID2);
      // Test not rebalancing.
      test_assert(add_child(sibling, root, i) == -1);
   }
   for (int i = 0; i < 4; i++) {
      rbnode_t * parent = root->children[i]; 
      rbnode_t * node = new_rbnode(&VOID3, VOID3);
      // Test case rebalance-rotate1 (all siblings red).
      test_assert(add_child(node, parent, rbinvert[side(parent)]) == 1);
   }
   // Destroy example tree and create it anew.
   destroy_rbnode(root);
   root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 3; i++) {
      sibling = new_rbnode(&VOID2, VOID2);
      // Test not rebalancing.
      test_assert(add_child(sibling, root, i) == -1);
   }
   root->children[2]->color = BLACK;
   for (int i = 0; i < 3; i++) {
      parent = root->children[i]; 
      node = new_rbnode(&VOID3, VOID3);
      // Test case rebalance-rotate1 (all siblings red).
      if (i < 2) {
         test_assert(add_child(node, parent, rbinvert[side(parent)]) == 1);
      } else { // Rebalance not triggered.
         test_assert(add_child(node, parent, rbinvert[side(parent)]) == -1);
      }
   }
   destroy_rbnode(root);
}

void
test_rotate1
(void)
{
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID2, VOID2);
      add_child(sibling, root, i);
   }
   for (int i = 0; i < 4; i++) {
      rbnode_t * parent = root->children[i]; 
      rbnode_t * node = new_rbnode(&VOID3, VOID3);
      add_child(node, parent, rbinvert[side(parent)]);
      // Test case rotate1 (all siblings red).
      test_assert(root->children[i] == node);
      test_assert(node->children[side(node)] == parent);
      test_assert(node->parent == root);
      test_assert(parent->parent == node);
      test_assert(side(node) == side(parent));
      test_assert(root->color == BLACK);
      test_assert(node->color == RED);
      test_assert(parent->color == RED);
   }
   // Destroy example tree and create it anew.
   destroy_rbnode(root);
   root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 3; i++) {
      sibling = new_rbnode(&VOID2, VOID2);
      add_child(sibling, root, i);
   }
   root->children[2]->color = BLACK;
   for (int i = 0; i < 3; i++) {
      parent = root->children[i]; 
      node = new_rbnode(&VOID3, VOID3);
      add_child(node, parent, rbinvert[side(parent)]);
      // Test case rebalance-rotate1 (all siblings red).
      if (i < 2) {
         test_assert(root->children[i] == node);
         test_assert(node->children[side(node)] == parent);
         test_assert(node->parent == root);
         test_assert(parent->parent == node);
         test_assert(side(node) == side(parent));
         test_assert(root->color == BLACK);
         test_assert(node->color == RED);
         test_assert(parent->color == RED);
      } else { // Rebalance not triggered.
         test_assert(root->children[i] == parent);
         test_assert(parent->children[side(parent)] == node);
         test_assert(parent->parent == root);
         test_assert(node->parent == parent);
         test_assert(side(node) == rbinvert[side(parent)]);
         test_assert(root->color == BLACK);
         test_assert(parent->color == BLACK);
         test_assert(node->color == RED);
      }
   }
   destroy_rbnode(root);
}

void
test_rebalancing_rotate2
(void)
{
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID2, VOID2);
      // Test not rebalancing.
      test_assert(add_child(sibling, root, i) == -1);
      if (i != 0) sibling->color == BLACK;
   }
   rbnode_t * parent = root->children[0]; 
   rbnode_t * node = new_rbnode(&VOID3, VOID3);
   // Test case rebalancing-rotate2 (all siblings black).
   test_assert(add_child(node, parent, rbinvert[side(parent)]) == 2);
   while (root->parent != NULL) root = root->parent;
   // Destroy example tree and create it anew.
   destroy_rbnode(root);
   root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   for (int i = 0; i < 3; i++) {
      rbnode_t * sibling = new_rbnode(&VOID2, VOID2);
      // Test not rebalancing.
      test_assert(add_child(sibling, root, i) == -1);
      if (i != 0) sibling->color == BLACK;
   }
   parent = root->children[0]; 
   node = new_rbnode(&VOID3, VOID3);
   // Test case rebalancing-rotate2 (all siblings black).
   test_assert(add_child(node, parent, rbinvert[side(parent)]) == 2);
   while (root->parent != NULL) root = root->parent;
   destroy_rbnode(root);
}

void
test_rotate2
(void)
{
   rbnode_t * root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   rbnode_t * grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   add_child(grandpa, root, 0);
   for (int i = 0; i < 4; i++) {
      rbnode_t * sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
      if (i != 0) sibling->color == BLACK;
   }
   rbnode_t * parent = grandpa->children[0]; 
   rbnode_t * node = new_rbnode(&VOID3, VOID3);
   add_child(node, parent, rbinvert[side(parent)]);
   // Test case rebalancing-rotate2 (all siblings black).
   test_assert(root->children[0] == parent);
   test_assert(grandpa->children[0] == NULL);
   test_assert(parent->children[0] == node);
   test_assert(parent->children[3] == grandpa);
   test_assert(grandpa->parent == parent);
   test_assert(parent->parent == root);
   test_assert(node->parent == parent);
   test_assert(side(grandpa == rbinvert[side(parent)]));
   test_assert(side(parent) == 0);
   test_assert(side(node) == side(parent));
   test_assert(root->color == BLACK);
   test_assert(grandpa->color == RED);
   test_assert(parent->color == BLACK);
   test_assert(node->color == RED);
   while (root->parent != NULL) root = root->parent;
   // Destroy example tree and create it anew.
   destroy_rbnode(root);
   root = new_rbnode(&VOID1, VOID1);
   root->color = BLACK;
   rbnode_t * grandpa = new_rbnode(&VOID2, VOID2);
   grandpa->color = BLACK;
   add_child(grandpa, root, 0);
   for (int i = 0; i < 3; i++) {
      rbnode_t * sibling = new_rbnode(&VOID3, VOID3);
      add_child(sibling, grandpa, i);
      if (i != 0) sibling->color == BLACK;
   }
   parent = grandpa->children[0]; 
   node = new_rbnode(&VOID3, VOID3);
   add_child(node, parent, rbinvert[side(parent)]);
   // Test case rebalancing-rotate2 (all siblings black).
   test_assert(root->children[0] == parent);
   test_assert(grandpa->children[0] == NULL);
   test_assert(parent->children[0] == node);
   test_assert(parent->children[3] == grandpa);
   test_assert(grandpa->parent == parent);
   test_assert(parent->parent == root);
   test_assert(node->parent == parent);
   test_assert(side(grandpa == rbinvert[side(parent)]));
   test_assert(side(parent) == 0);
   test_assert(side(node) == side(parent));
   test_assert(root->color == BLACK);
   test_assert(grandpa->color == RED);
   test_assert(parent->color == BLACK);
   test_assert(node->color == RED);
   while (root->parent != NULL) root = root->parent;
   destroy_rbnode(root);
}

void
test_size
(void)
{
   for (int i = 0; i < 4; i++) {
      rbnode_t * parent = new_rbnode(&VOID1, VOID1);
      rbnode_t * node = new_rbnode(&VOID2, VOID2);
      test_assert(side(node) == INT_MAX);
      add_child(node, parent, i);
      test_assert(side(node) == i);
      test_assert(parent->children[i] == node);
      parent->children[i] == NULL;
      test_assert(side(node) == INT_MAX);
      destroy_rbnode(parent);
   }
}

int
main(
   int argc,
   char **argv
)
{

   // Register test cases //
   const static test_case_t test_cases[] = {
      {"test_node_creation_destruction", test_node_creation_destruction},
      {"test_node_destruction", test_node_destruction},
      {"test_child_addition", test_child_addition},
      {"test_rebalancing_propagate", test_rebalancing_propagate},
      {"test_propagate", test_propagate},
      {"test_rebalancing_rotate1", test_rebalancing_rotate1},
      {"test_rotate1", test_rotate1},
      {"test_rebalancing_rotate2", test_rebalancing_rotate2},
      {"test_rotate2", test_rotate2},
      {"test_size", test_size},
      {NULL, NULL}
   };

   return run_unittest(argc, argv, test_cases);

}

