#include "view.h"

void
force_directed_drawing
(
   int       n_balls,
   ball_t ** ball_list,
   int       maxthreads
)
{
   // Initialize ball positions.
   // Define canvas size and generate random positions in it.
   int canvas_size[2] = { CANVAS_SIZE , CANVAS_SIZE };
   srand(time(NULL));
   fprintf(stderr, "initializing\n");
   int n_stars = 0;
   gstack_t * star_list = list_stars(n_balls, ball_list, &n_stars);
   //for (int i = 0; i < NTHREADS; i++) {
   //   int start = i * n_balls / NTHREADS;
   //   int stop = (i+1) * n_balls / NTHREADS;
   //   if (stop > n_balls) stop = n_balls; 
   //   initialize_positions(start, stop, ball_list);
   //}
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
      fprintf(stderr, "new loop\n");
      physics_loop(n_balls, ball_list, moves, movement_list);
      regression(moves, movement_list, regr);
   } while (regr[0] < flat || regr[1] > 10.0);

   fprintf(stderr, "spiralize\n");
   // Define stars and bubble-plot them.
   //int n_stars = 0;
   //star_t ** star_list = list_stars(n_balls, ball_list, &n_stars);
   qsort(star_list, n_stars, sizeof(ball_t *), compar);
   spiralize_displacements(n_stars, star_list, canvas_size);
   move_stars(n_balls, n_stars, ball_list, star_list);

   fprintf(stderr, "draw\n");
   // Draw all balls and bonds.
   int offset[2] = { 0, 0 };
   resize_canvas(canvas_size, n_stars, star_list, offset);
   cairo_surface_t * surface;
   surface = cairo_pdf_surface_create(
         "example_starcode_image.pdf", canvas_size[0], canvas_size[1]);
   cairo_t * cr = cairo_create(surface);
   draw_cairo_env(cr, n_balls, ball_list, offset);
   cairo_surface_finish(surface);

   // Free memory.
   free(star_list);
   fprintf(stderr, "done\n");
}

//void
//initialize_positions
//(
//   int       start,
//   int       stop,
//   ball_t ** ball_list
//)
//{
//   for (int j = start; j < stop; j++) {
//      ball_list[j]->position[0] = rand() * RAND_FACTOR;
//      ball_list[j]->position[1] = rand() * RAND_FACTOR;
//   }
//}

double
norm2
(
   double x_coord,
   double y_coord
)
{
   return x_coord * x_coord + y_coord * y_coord;
}

double
norm
(
   double x_coord,
   double y_coord
)
{
   return sqrt(norm2(x_coord, y_coord));
}

double
electric
(
   ball_t * ball1,
   ball_t * ball2,
   double   dist2
)
{
   double ke = 5.0e1;
   // Coulomb's law.
   return ke * ball1->size * ball2->size / dist2;
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
   double v_norm2 = norm2(x_dist, y_dist);
   double v_norm = sqrt(v_norm2);
   double force = 0.0;
   if (force_type == 0) {
      force = elastic(v_norm);
   } else if (force_type == 1) {
      force = electric(ball1, ball2, v_norm2);
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
         int n_children = ball->children->nitems;
         for (int j = 0; j < n_children; j++) {
            ball_t * child = (ball_t *) ball->children->items[j];
            compute_force(ball, child, 0);
         }
         // ...and electric forces.
         for (int k = i+1; k < n_balls; k++) {
            // Isolate the physics of different stars.
            if (ball->starid == ball_list[k]->starid) {
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
      double p = movement_list[i] - x_mean;
      x  += p * p;
      xy += p * (i - y_mean);
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

gstack_t *
list_stars
(
   int       n_balls,
   ball_t ** ball_list,
   int     * n_stars
)
{
   *n_stars = 0;
   // Create a star list and push stars into it.
   gstack_t * star_list = new_gstack();
   for (int i = 0; i < n_balls; i++) {
      // Create a star and initialize it.
      if (ball_list[i]->starid == i+1) { // TODO: why is that comparison for?
         star_t * star = malloc(sizeof(star_t));
         if (star == NULL) {
            fprintf(stderr, "Error in star_t malloc: %s\n", strerror(errno));
         }
         star->starid = ball_list[i]->starid;
         star->position[0] = ball_list[i]->position[0];
         star->position[1] = ball_list[i]->position[1];
         star->members = new_gstack();
         push((void *) star, &star_list);
         (*n_stars)++;
      } else if (0) { // TODO: condition to add members to the star
         push((void *) ball_list[i], $star_list[ball_list[i]->starid]);
      }
   }
   // Define stars (center) position and radius.
   for (int i = 0; i < *n_stars; i++) {
      int star_size = 0;
      double mean_pos[2] = { 0.0, 0.0 };
      for (int j = 0; j < n_balls; j++) {
         if (star_list[i]->starid == ball_list[j]->starid) {
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
      for (int j = 0; j < n_balls; j++) {
         ball_t * ball = ball_list[j];
         if (star_list[i]->starid == ball->starid) {
            double x_dist = star_list[i]->position[0] - ball->position[0];
            double y_dist = star_list[i]->position[1] - ball->position[1];
            double radius = norm(x_dist, y_dist) + sqrt(ball->size / PI);
            if (radius > star_list[i]->radius) star_list[i]->radius = radius;
         }
      }
   }
   return star_list;
}

/*
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
      if (ball_list[i]->starid == i+1) {
         star_list[*n_stars] = malloc(sizeof(star_t));
         if (star_list[*n_stars] == NULL) {
            fprintf(stderr, "Error in star_list malloc: %s\n",
                    strerror(errno));
         }
         star_list[*n_stars]->starid = ball_list[i]->starid;
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
         if (star_list[i]->starid == ball_list[j]->starid) {
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
      for (int j = 0; j < n_balls; j++) {
         ball_t * ball = ball_list[j];
         if (star_list[i]->starid == ball->starid) {
            double x_dist = star_list[i]->position[0] - ball->position[0];
            double y_dist = star_list[i]->position[1] - ball->position[1];
            double radius = norm(x_dist, y_dist) + sqrt(ball->size / PI);
            if (radius > star_list[i]->radius) star_list[i]->radius = radius;
         }
      }
   }
   return star_list;
}
*/

void
spiralize_displacements
(
   int       n_stars, 
   star_t ** star_list,
   int     * canvas_size
)
{
   double center[2] = { canvas_size[0] / 2.0, canvas_size[1] / 2.0 };
   double step = 0.1; // Step along the spiral and padding between stars.
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
         if (ball_list[i]->starid == star_list[j]->starid) {
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
   for (int j = 0; j < ball->children->nitems; j++) {
      ball_t * child = (ball_t *) ball->children->items[j];
      double child_x_pos = child->position[0] - offset[0];
      double child_y_pos = child->position[1] - offset[1];
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
   // And then the graphs.
   cairo_set_line_width(cr, 0.1);
   cairo_set_source_rgb(cr, 0, 0, 0);
   for (int i = 0; i < n_balls; i++) draw_edges(cr, ball_list[i], offset);
   for (int i = 0; i < n_balls; i++) draw_circles(cr, ball_list[i], offset);
}
