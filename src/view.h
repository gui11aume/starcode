#include <cairo.h>
#include <cairo-pdf.h>
#include <errno.h>
#include <execinfo.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "trie.h"

#define BALL_SIZE(elem) 2 * sizeof(int)   + \
                        4 * sizeof(double) + \
                  (elem+1)* sizeof(ball_t *)
#define CANVAS_SIZE 600.0
#define RAND_FACTOR CANVAS_SIZE / RAND_MAX
#define PI 3.141592653589793238462643383279502884L
#define NTHREADS 6

struct ball_t;
struct star_t;

typedef struct ball_t ball_t;
typedef struct star_t star_t;

struct ball_t {
   int        starid;
   double     size;
   double     position[2];
   double     force[2];
   gstack_t * children;
};


struct star_t {
   int        starid;          // Numeric ID of the star.
   double     position[2];     // (x,y) initial position of the root.
   double     displacement[2]; // (x,y) distance from position.
   double     radius;          // Distance to most distant ball + its radius.
   gstack_t * members;         // Balls inside star.
};

gstack_t * list_stars(int, ball_t **, int *);
//void      initialize_positions(int, int, ball_t **);
void       force_directed_drawing(int, ball_t **, int);
double     norm(double, double);
double     electric(ball_t *, ball_t *, double);
double     elastic(double);
void       compute_force(ball_t *, ball_t *, int);
double     move_ball(ball_t *);
void       physics_loop(int, ball_t **, int, double *);
void       regression(int, double *, double *);
int        compar(const void *, const void *);
//star_t ** list_stars(int, ball_t **, int *);
void       spiralize_displacements(int, star_t **, int *);
void       move_stars(int, int, ball_t **, star_t **);
void       resize_canvas(int *, int, star_t **, int *);
void       draw_cairo_env(cairo_t *, int, ball_t **, int *);
void       draw_edges(cairo_t *, ball_t *, int *);
void       draw_circles(cairo_t *, ball_t *, int *);
