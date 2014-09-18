#define RED 0
#define BLACK 1
#define IB (uint32_t) 0 // IN-BEFORE
#define IA (uint32_t) 1 // IN-AFTER
#define OB (uint32_t) 2 // OUT-BEFORE
#define OA (uint32_t) 3 // OUT-AFTER

const uint32_t mask = 3; // 2-bit mask for obtaining the
                         // opposite node during rotations.
struct rbnode_t;

typedef struct rbnode_t rbnode_t;

struct rbnode_t {
   int        color;
   int        side;
   void     * box;
   rbnode_t * parent;
   rbnode_t * children[4];
};

void add_child(rbnode_t *, void *, int);
void rebalance(rbnode_t *, int);
