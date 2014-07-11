#define BALL_SIZE(elem) 2 * sizeof(int)   + \
                        4 * sizeof(double) + \
                  (elem+1)* sizeof(ball_t *)
#define CANVAS_SIZE 600.0
#define RAND_FACTOR CANVAS_SIZE / RAND_MAX
#define PI 3.141592653589793238462643383279502884L

struct ball_t;
struct star_t;

typedef struct ball_t ball_t;
typedef struct star_t star_t;

struct ball_t {
   int      size;        // # of barcodes
   int      n_children;  // # of children
   double   position[2]; // (x,y)
   double   force[2];    // (x,y)
   ball_t * root;        // address to root
   ball_t * children[];  // list of direct children
};

struct star_t {
   ball_t * root;            // address of root ball_t
   double   position[2];     // (x,y) initial position of the root
   double   displacement[2]; // (x,y) distance from position
   double   radius;          // distance to most distant ball + its radius
};

void      force_directed_drawing(int, ball_t **);
//ball_t ** list_balls(FILE *, int *);
//ball_t ** new_ball(char *);
double    norm(double, double);
double    electric(ball_t *, ball_t *, double);
double    elastic(double);
void      compute_force(ball_t *, ball_t *, int);
double    move_ball(ball_t *);
void      physics_loop(int, ball_t **, int, double *);
void      regression(int, double *, double *);
int       compar(const void *, const void *);
star_t ** list_stars(int, ball_t **, int *);
void      spiralize_displacements(int, star_t **, int *);
void      move_stars(int, int, ball_t **, star_t **);
void      resize_canvas(int *, int, star_t **, int *);
void      draw_cairo_env(cairo_t *, int, ball_t **, int *);
void      draw_edges(cairo_t *, ball_t *, int *);
void      draw_circles(cairo_t *, ball_t *, int *);
