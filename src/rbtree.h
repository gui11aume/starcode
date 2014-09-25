#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum { RED, BLACK } color_t;
typedef enum { SW, NW, SE, NE } side_t;
const side_t invert[4] = { NE, SE, NW, SW };

static int ERROR = 0;

struct rbnode_t;

typedef struct rbnode_t rbnode_t;

struct rbnode_t {
   color_t    color;
   double     xy[2];
   void     * box;
   rbnode_t * parent;
   rbnode_t * children[4];
};

rbnode_t * new_rbnode(void *, double *);
void       destroy_rbnode(rbnode_t *);
void       add_child(rbnode_t *, rbnode_t *, side_t);
void       rebalance(rbnode_t *);
rbnode_t * case_rotate1(rbnode_t *);
void       case_rotate2(rbnode_t *);
void       case_propagate(rbnode_t *);
side_t     side(rbnode_t *);
