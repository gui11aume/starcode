#define RED 0
#define BLACK 1
#define SW (uint32_t) 0 // SOUTH-WEST
#define NW (uint32_t) 1 // NORTH-WEST
#define SE (uint32_t) 2 // SOUTH-EAST
#define NE (uint32_t) 3 // NORTH-EAST

static int ERROR = 0;
const uint32_t MASK = 3; // 2-bit mask for obtaining the
                         // opposite node during rotations.
struct rbnode_t;

typedef struct rbnode_t rbnode_t;

struct rbnode_t {
   int        color;
   int        side;
   double     xy[2];
   void     * box;
   rbnode_t * parent;
   rbnode_t * children[4];
};

rbnode_t * new_rbnode(void *, double *);
void destroy_rbnode(rbnode_t *);
void add_child(rbnode_t *, rbnode_t *, uint32_t);
void rebalance(rbnode_t *);
void case_rotate1(rbnode_t *);
void case_rotate2(rbnode_t *);
void case_propagate(rbnode_t *);
uint32_t invert(uint32_t);
