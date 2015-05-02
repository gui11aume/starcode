/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
**
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

#define VERSION          "starcode-v1.0"
#define STARCODE_MAX_TAU 8

typedef enum {
   DEFAULT_OUTPUT,
   SPHERES_OUTPUT,
   PRINT_NRED,
   ALL_PAIRS
} output_t;

int starcode(
         FILE * inputf1,
         FILE * inputf2,
         FILE * outputf1,
   const int    tau,
   const int    verbose,
         int    maxthreads
);

#endif
