#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include "starcode.h"
#include "view.h"

void SIGSEGV_handler(int sig) {
   void *array[10];
   size_t size;

   // get void*'s for all entries on the stack
   size = backtrace(array, 10);
            
   // print out all the frames to stderr
   fprintf(stderr, "Error: signal %d:\n", sig);
   backtrace_symbols_fd(array, size, STDERR_FILENO);
   exit(1);
}

int main(int argc, const char *argv[])
{
   signal(SIGSEGV, SIGSEGV_handler);

   // Create list of ball pointers.
   //FILE * inputf = fopen("node_network.txt", "r");
   //int n_balls;
   //ball_t * ball_list = list_balls(inputf, &n_balls);

   /* Begin test code */
   const int n_balls = 8;
   ball_t * ball_list[8]; // in the stack
   ball_list[0] = malloc(BALL_SIZE(3));
   ball_list[1] = malloc(BALL_SIZE(0));
   ball_list[2] = malloc(BALL_SIZE(0));
   ball_list[3] = malloc(BALL_SIZE(0));
   ball_list[4] = malloc(BALL_SIZE(2));
   ball_list[5] = malloc(BALL_SIZE(1));
   ball_list[6] = malloc(BALL_SIZE(0));
   ball_list[7] = malloc(BALL_SIZE(0));

   // star 1
   ball_list[0]->size = 1000;
   ball_list[0]->n_children = 3;
   ball_list[0]->root = ball_list[0];
   ball_list[0]->children[0] = ball_list[1];
   ball_list[0]->children[1] = ball_list[2];
   ball_list[0]->children[2] = ball_list[3];

   ball_list[1]->size = 10;
   ball_list[1]->n_children = 0;
   ball_list[1]->root = ball_list[0];
   
   ball_list[2]->size = 5;
   ball_list[2]->n_children = 0;
   ball_list[2]->root = ball_list[0];

   ball_list[3]->size = 30;
   ball_list[3]->n_children = 0;
   ball_list[3]->root = ball_list[0];

   // star 2
   ball_list[4]->size = 1500;
   ball_list[4]->n_children = 2;
   ball_list[4]->root = ball_list[4];
   ball_list[4]->children[0] = ball_list[5];
   ball_list[4]->children[1] = ball_list[6];

   ball_list[5]->size = 20;
   ball_list[5]->n_children = 1;
   ball_list[5]->root = ball_list[4];
   ball_list[5]->children[0] = ball_list[7];

   ball_list[6]->size = 15;
   ball_list[6]->n_children = 0;
   ball_list[6]->root = ball_list[4];

   ball_list[7]->size = 8;
   ball_list[7]->n_children = 0;
   ball_list[7]->root = ball_list[4];
   /* End test code */

   force_directed_drawing(n_balls, ball_list);

   return 0;
}

void
force_directed_drawing
(
   int       n_balls,
   ball_t ** ball_list
)
{
   // Initialize ball positions.
   // Define canvas size and generate random positions in it.
   int canvas_size[2] = { CANVAS_SIZE , CANVAS_SIZE };
   srand(time(NULL));
   for (int i = 0; i < n_balls; i++) {
      ball_list[i]->position[0] = rand() * RAND_FACTOR;
      ball_list[i]->position[1] = rand() * RAND_FACTOR;
   }

   // Initialize and run physics loop.
   int    moves = 200;
   double movement_list[moves];
   double flat = 1e-1;
   double regr[2];
   do {
      physics_loop(n_balls, ball_list, moves, movement_list);
      regression(moves, movement_list, regr);
   } while (regr[0] < flat || regr[1] > 10.0);

   // Define stars and bubble-plot them.
   int n_stars = 0;
   star_t ** star_list = list_stars(n_balls, ball_list, &n_stars);
   qsort(star_list, n_stars, sizeof(ball_t *), compar);
   spiralize_displacements(n_stars, star_list, canvas_size);
   move_stars(n_balls, n_stars, ball_list, star_list);

   // Draw all balls and bonds.
   int offset[2] = { 0, 0 };
   resize_canvas(canvas_size, n_stars, star_list, offset);
   cairo_surface_t * surface;
   surface = cairo_pdf_surface_create(
         "example_starcode_image.pdf", canvas_size[0], canvas_size[1]);
   cairo_t * cr = cairo_create(surface);
   draw_cairo_env(cr, n_balls, ball_list, offset);
   cairo_surface_finish(surface);
}

//ball_t *
//list_balls
//(
//   FILE * inputf,
//   int  * n_balls
//)
//{
//   *n_balls = 0;
//   size_t list_size = 1024;
//   ball_t * ball_list = malloc(list_size * sizeof(ball_t *));
//   if (ball_list == NULL) {
//      fprintf(stderr, "Error in ball_list malloc: %s\n", strerror(errno));
//   }
//   size_t nchar = M;
//   char * line = malloc(M * sizeof(char));
//   if (line == NULL) {
//      fprintf(stderr, "Error in line malloc: %s\n",strerror(errno));
//   }
//   ssize_t nread;
//   while ((nread = getline(&line, &nchar, inputf)) != -1) {
//      // Strip end of line character.
//      if (line[nread-1] == '\n') line[nread-1] = '\0';
//      ball_t * ball = new_ball(line);
//      n_balls++;
//      if (n_balls >= list_size) {
//         list_size *= 2;
//         ball_list = realloc(ball_list, list_size * sizeof(ball_t *));
//         if (ball_list == NULL) {
//            fprintf(stderr, "Error in ball_list realloc: %s\n",
//                    strerror(errno));
//         }
//      }
//   }
//   free(line);
//   if (fclose(inputf) == EOF) {
//      fprintf(stderr, "Error in closing input file: %s\n", strerror(errno));
//   }
//   return ball_list;
//}
//
//ball_t *
//new_ball
//(
//   char * line
//)
//{
//   char * child;
//   char * parent;
//   char * root;
//   if (sscanf(line, "%s\t%d
//                     %s\t%d
//                     %s\t%d",
//                     copy, &count
//                     copy, &count
//                     copy, &count) == 6) seq = copy;
//   else {
//
//   }
//   ball_t * ball = malloc(BALL_SIZE(n_children));
//   if (ball == NULL) {
//      fprintf(stderr, "Error in ball malloc: %s\n", strerror(errno));
//   }
//   // TODO: fill ball with info.
//
//   return ball;
//}

double
norm
(
   double x_coord,
   double y_coord
)
{
   return sqrt(pow(x_coord, 2) + pow(y_coord, 2));
}

double
electric
(
   ball_t * ball1,
   ball_t * ball2,
   double   dist
)
{
   double ke = 5.0e1;
   // Coulomb's law.
   return ke * ball1->size * ball2->size / pow(dist, 2);
}

double
elastic
(
   double dist
)
{
   double k = 10.0;
   // Hooke's law.
   return -k * dist;
}

void
compute_force
(
   ball_t * ball1,
   ball_t * ball2,
   int      force_type
)
{
   // Relative ball positions determine the sign (+/-) of the unit vector.
   double x_dist = ball2->position[0] - ball1->position[0];
   double y_dist = ball2->position[1] - ball1->position[1];
   double v_norm = norm(x_dist, y_dist);
   double force;
   if (force_type == 0) {
      force = elastic(v_norm);
   } else if (force_type == 1) {
      force = electric(ball1, ball2, v_norm);
   }
   double u_vect[2] = { x_dist / v_norm, y_dist / v_norm };
   double x_force = force * u_vect[0];
   double y_force = force * u_vect[1];
   ball1->force[0] += -x_force;
   ball2->force[0] +=  x_force;
   ball1->force[1] += -y_force;
   ball2->force[1] +=  y_force;
}

double
move_ball
(
   ball_t * ball
)
{
   double x_movement = ball->force[0] / ball->size;
   double y_movement = ball->force[1] / ball->size;
   // Limit movement to a "terminal velocity" to prevent diverging.
   double tv = 1;
   if (x_movement > tv) x_movement = tv;
   else if (x_movement < -tv) x_movement = -tv;
   if (y_movement > tv) y_movement = tv;
   else if (y_movement < -tv) y_movement = -tv;
   ball->position[0] += x_movement;
   ball->position[1] += y_movement;
   return norm(x_movement, y_movement);
}

void
physics_loop
(
   int       n_balls,
   ball_t ** ball_list,
   int       moves,
   double  * movement_list
)
{
   for (int k = 0; k < moves; k++) {
      // Reinitialize forces at each iteration.
      for (int i = 0; i < n_balls; i++) {
         ball_list[i]->force[0] = 0.0;
         ball_list[i]->force[1] = 0.0;
      }
      // Compute the forces among the balls.
      for (int i = 0; i < n_balls; i++) {
         ball_t * ball = ball_list[i];
         // Compute elastic... 
         for (int j = 0; j < ball->n_children; j++) {
            compute_force(ball, ball->children[j], 0);
         }
         // ...and electric forces.
         for (int k = i+1; k < n_balls; k++) {
            // Isolate the physics of different stars.
            if (ball->root == ball_list[k]->root) {
               compute_force(ball, ball_list[k], 1);
            }
         }
      }
      movement_list[k] = 0.0;
      for (int i = 0; i < n_balls; i++) {
         movement_list[k] += move_ball(ball_list[i]);
      }
   }
}

void
regression
(
   int      moves,
   double * movement_list,
   double * regression
)
{
   double x  = 0.0;
   double xy = 0.0;
   double x_mean = 0.0;
   double y_mean = (moves + 1) / 2.0;
   for (int i = 0; i < moves; i++) x_mean += movement_list[i];
   x_mean /= moves;
   for (int i = 0; i < moves; i++) {
      x  += pow(movement_list[i] - x_mean, 2);
      xy += (movement_list[i] - x_mean) * (i - y_mean);
   }
   regression[0] = xy / x;                          // Slope.
   regression[1] = y_mean - regression[0] * x_mean; // Intercept.
}

int
compar
(
   const void * elem1,
   const void * elem2
)
{
   ball_t * ball1 = (ball_t *) elem1;
   ball_t * ball2 = (ball_t *) elem2;
   return (ball1->size > ball2->size) ? -1 : 1; // Descending order.
}

star_t **
list_stars
(
   int       n_balls,
   ball_t ** ball_list,
   int     * n_stars
)
{
   *n_stars = 0;
   int list_size  = 1000;
   star_t ** star_list = malloc(list_size * sizeof(star_t *));
   if (star_list == NULL) {
      fprintf(stderr, "Error in star malloc: %s\n", strerror(errno));
   }
   // Define stars root identity and position.
   for (int i = 0; i < n_balls; i++) {
      if (ball_list[i] == ball_list[i]->root) {
         star_list[*n_stars] = malloc(sizeof(star_t));
         if (star_list[*n_stars] == NULL) {
            fprintf(stderr, "Error in star_list malloc: %s\n",
                    strerror(errno));
         }
         star_list[*n_stars]->root = ball_list[i]->root;
         star_list[*n_stars]->position[0] = ball_list[i]->position[0];
         star_list[*n_stars]->position[1] = ball_list[i]->position[1];
         (*n_stars)++;
         if (*n_stars >= list_size) {
            list_size *= 2;
            star_list =
               realloc(star_list, list_size * sizeof(star_t *));
            if (star_list == NULL) {
               fprintf(stderr, "Error in star_list realloc: %s\n",
                       strerror(errno));
            }
         }
      }
   }
   // Define stars (center) position and radius.
   for (int i = 0; i < *n_stars; i++) {
      int star_size = 0;
      double mean_pos[2] = { 0.0, 0.0 };
      for (int j = 0; j < n_balls; j++) {
         if (star_list[i]->root == ball_list[j]->root) {
            mean_pos[0] += ball_list[j]->position[0];
            mean_pos[1] += ball_list[j]->position[1];
            star_size++;
         } 
      }
      // Now position is the central position of the star.
      star_list[i]->position[0] = mean_pos[0] / star_size;
      star_list[i]->position[1] = mean_pos[1] / star_size;
      // Compute the radius.
      star_list[i]->radius = 0.0;
      double radius = 0.0;
      for (int j = 0; j < n_balls; j++) {
         ball_t * ball = ball_list[j];
         if (star_list[i]->root == ball->root) {
            double x_dist = star_list[i]->position[0] - ball->position[0];
            double y_dist = star_list[i]->position[1] - ball->position[1];
            double radius = norm(x_dist, y_dist) + sqrt(ball->size / PI);
            if (radius > star_list[i]->radius) star_list[i]->radius = radius;
         }
      }
   }
   return star_list;
}

void
spiralize_displacements
(
   int       n_stars, 
   star_t ** star_list,
   int     * canvas_size
)
{
   double center[2] = { canvas_size[0] / 2.0, canvas_size[1] / 2.0 };
   double step = 0.01; // Step along the spiral and padding between stars.
   // Place the first star in the center of the canvas.
   star_list[0]->displacement[0] = center[0] - star_list[0]->position[0];
   star_list[0]->displacement[1] = center[1] - star_list[0]->position[1];
   star_list[0]->position[0] = center[0];
   star_list[0]->position[1] = center[1];
   for (int i = 1; i < n_stars; i++) {
      star_t * star1 = star_list[i];
      double x_pos;
      double y_pos;
      double distance = 0.0; // Distance from center, along a spiral line.
      double phase = 2 * PI * rand() / RAND_MAX;
      int overlap = 1;
      while (overlap) {
         overlap = 0;
         x_pos = center[0] + distance * cos(distance + phase); 
         y_pos = center[1] + distance * sin(distance + phase); 
         for (int j = 0; j < i; j++) {
            star_t * star2 = star_list[j];
            double x_dist = x_pos - star2->position[0];
            double y_dist = y_pos - star2->position[1];
            double radii = star1->radius + star2->radius;
            if (norm(x_dist, y_dist) - radii < step) {
               overlap = 1;
               distance += step;
               break;
            }
         }
      }
      star1->displacement[0] = x_pos - star1->position[0];
      star1->displacement[1] = y_pos - star1->position[1];
      star1->position[0] = x_pos;
      star1->position[1] = y_pos;
   }
}

void
move_stars
(
   int       n_balls,
   int       n_stars,
   ball_t ** ball_list,
   star_t ** star_list
)
{
   for (int i = 0; i < n_balls; i++) {
      for (int j = 0; j < n_stars; j++) {
         if (ball_list[i]->root == star_list[j]->root) {
            ball_list[i]->position[0] += star_list[j]->displacement[0];
            ball_list[i]->position[1] += star_list[j]->displacement[1];
         }
      }
   }
}

void
resize_canvas
(
   int     * canvas_size,
   int       n_stars,
   star_t ** star_list,
   int     * offset
)
{
   double x_max = -INFINITY;
   double x_min =  INFINITY;
   double y_max = -INFINITY;
   double y_min =  INFINITY;
   for (int i = 0; i < n_stars; i++) {
      star_t * star = star_list[i];
      int x_dist = star->position[0];
      int y_dist = star->position[1];
      if (x_dist + star->radius > x_max) x_max = x_dist + star->radius;
      if (x_dist - star->radius < x_min) x_min = x_dist - star->radius;
      if (y_dist + star->radius > y_max) y_max = y_dist + star->radius;
      if (y_dist - star->radius < y_min) y_min = y_dist - star->radius;
   }
   offset[0] = (int) x_min;
   offset[1] = (int) y_min;
   canvas_size[0] = (int) (x_max - x_min) + 5;
   canvas_size[1] = (int) (y_max - y_min) + 5;
}

void
draw_edges
(
   cairo_t * cr,
   ball_t  * ball,
   int     * offset
)
{
   double x_pos = ball->position[0] - offset[0];
   double y_pos = ball->position[1] - offset[1];
   for (int j = 0; j < ball->n_children; j++) {
      double child_x_pos = ball->children[j]->position[0] - offset[0];
      double child_y_pos = ball->children[j]->position[1] - offset[1];
      cairo_set_line_width(cr, 1);
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_move_to(cr, x_pos, y_pos);
      cairo_line_to(cr, child_x_pos, child_y_pos);
      cairo_stroke(cr);
   }
}

void
draw_circles
(
   cairo_t * cr,
   ball_t  * ball,
   int     * offset
)
{
   double x_pos = ball->position[0] - offset[0];
   double y_pos = ball->position[1] - offset[1];
   double radius = sqrt(ball->size / PI);
   // Draw the circle.
   cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
   cairo_arc(cr, x_pos, y_pos, radius, 0, 2*PI);
   cairo_close_path(cr);
   cairo_fill(cr);
   // And the outline.
   cairo_set_source_rgb(cr, 0, 0, 0);
   cairo_arc(cr, x_pos, y_pos, radius, 0, 2*PI);
   cairo_close_path(cr);
   cairo_stroke(cr);
}

void
draw_cairo_env
(
   cairo_t * cr,
   int       n_balls,
   ball_t ** ball_list,
   int     * offset
)
{
   // Paint the background.
   cairo_set_source_rgb(cr, 1, 1, 1);
   cairo_paint(cr);
   for (int i = 0; i < n_balls; i++) {
      ball_t * ball = ball_list[i];
      // And then the graphs.
      draw_edges(cr, ball, offset);
      draw_circles(cr, ball, offset);
   }
}
