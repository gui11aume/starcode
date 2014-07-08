#define BALL_SIZE(elem) 2 * sizeof(int)   + \
                        4 * sizeof(double) + \
                  (elem+1)* sizeof(ball_t *)
#define WIDTH 600.0
#define RAND_FACTOR WIDTH / RAND_MAX
#define ASPECT_TYPE 0
#if ASPECT_TYPE == 0 
   #define ASPECT 4 / 3
#elif ASPECT_TYPE == 1
   #define ASPECT 16 / 9
#endif
#define PI 3.141592653589793238462643383279502884L

struct ball_t;
struct cluster_t;

typedef struct ball_t ball_t;
typedef struct cluster_t cluster_t;

struct ball_t {
   int      size;        // # of barcodes
   int      n_children;  // # of children
   double   position[2]; // (x,y)
   double   force[2];    // (x,y)
   ball_t * root;        // address to root
   ball_t * children[];  // list of direct children
};

struct cluster_t {
   ball_t * root;          // address of root ball_t
   double position[2];     // (x,y) initial position of the root
   double displacement[2]; // (x,y) distance from position
   double radius;          // distance to most distant ball + its radius
};

//ball_t ** create_ball_list(char * filename);
//ball_t ** new_ball(int n_children);
double norm(double, double);
double electric(ball_t *, ball_t *, double);
double elastic(double);
void compute_force(ball_t *, ball_t *, int);
double move_ball(ball_t *, double);
int compar(const void *, const void *);
cluster_t ** create_cluster_list(int, ball_t **, int *);
void spiralize_displacements(int, cluster_t **, int *);
void move_clusters(int, int, ball_t **, cluster_t **);
void measure_space(int, int *, ball_t **);
void draw_cairo_env(cairo_t *, int, ball_t **);
void draw_edges(cairo_t *, ball_t *);
void draw_circles(cairo_t *, ball_t *);
