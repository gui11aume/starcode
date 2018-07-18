/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (eduardvalera@gmail.com)
**
** License: 
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
*/

#ifndef _STARCODE_HEADER
#define _STARCODE_HEADER

#define _GNU_SOURCE
#include <stdio.h>

#define VERSION          "starcode-v1.3 17-07-2018"
#define STARCODE_MAX_TAU 8

typedef enum {
   DEFAULT_OUTPUT,
   CLUSTER_OUTPUT,
   NRED_OUTPUT
} output_t;

typedef enum {
   MP_CLUSTER,
   SPHERES_CLUSTER,
   COMPONENTS_CLUSTER
} cluster_t;

int starcode(
   FILE *inputf1,
   FILE *inputf2,
   FILE *outputf1,
   FILE *outputf2,
         int tau,
   const int verbose,
         int thrmax,
   const int clusteralg,
         double parent_to_child,
   const int showclusters,
   const int showids,
   const int outputt
);

#endif
