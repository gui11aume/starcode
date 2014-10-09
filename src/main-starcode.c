/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
**
** Last modified: July 8, 2014
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

#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "starcode.h"

char *USAGE = "Usage:\n"
"  starcode [-i] input [-o output]\n"
"    -v --verbose: verbose\n"
"    -d --dist: maximum Levenshtein distance (default 3)\n"
"    -t --threads: number of concurrent threads (default 1)\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)\n"
"\n"
"  mutually exclusive options\n"
"    -s --sphere: sphere clustering (default message passing)\n"
"       --print-pairs: print matching pairs (no clustering)\n"
"       --non-redundant: print non-redundant sequences (no clustering)\n";

void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }

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

int
main(
   int argc,
   char **argv
)
{
   // Backtrace handler
   signal(SIGSEGV, SIGSEGV_handler); 

   // Set flags.
   static int pp_flag = 0;
   static int nr_flag = 0;
   static int sp_flag = 0;
   static int vb_flag = 0;

   // Unset flags (value -1).
   int dist = -1;
   int threads = -1;

   // Unset options (value 'UNSET').
   char * const UNSET = "unset";
   char *input = UNSET;
   char *output = UNSET;

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"print-pairs",       no_argument,       &pp_flag,  1 },
         {"non-redundant",     no_argument,       &nr_flag,  1 },
         {"sphere-clustering", no_argument,       &sp_flag,  1 },
         {"verbose",           no_argument,       &vb_flag,  1 },
         {"dist",              required_argument,        0, 'd'},
         {"help",              no_argument,              0, 'h'},
         {"input",             required_argument,        0, 'i'},
         {"output",            required_argument,        0, 'o'},
         {"threads",           required_argument,        0, 't'},
         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "d:hi:o:st:v",
            long_options, &option_index);
 
      // Done parsing options? //
      if (c == -1) break;

      switch (c) {
      case 0:
         // A flag was set. //
         break;

      case 'd':
         if (dist < 0) {
            dist = atoi(optarg);
         }
         else {
            fprintf(stderr, "--distance set more than once\n\n");
            say_usage();
            return 1;
         }
         break;

      case 'h':
         // User asked for help. //
         say_usage();
         return 0;

      case 'i':
         if (input == UNSET) {
            input = optarg;
         }
         else {
            fprintf(stderr, "--input set more than once\n\n");
            say_usage();
            return 1;
         }
         break;

      case 'o':
         if (output == UNSET) {
            output = optarg;
         }
         else {
            fprintf(stderr, "--output set more than once\n\n");
            say_usage();
            return 1;
         }
         break;

      case 's':
         sp_flag = 1;
         break;

      case 't':
         if (threads < 0) {
            threads = atoi(optarg);
         }
         else {
            fprintf(stderr, "--thread set more than once\n\n");
            say_usage();
            return 1;
         }
         break;

      case 'v':
         vb_flag = 1;
         break;

      default:
         // Cannot parse. //
         say_usage();
         return 1;

      }

   }

   if (optind < argc) {
      if ((optind == argc - 1) && (input == UNSET)) {
         input = argv[optind];
      }
      else {
         fprintf(stderr, "too many options\n\n");
         say_usage();
         return 1;
      }
   }

   // Check options compatibility. //
   if (pp_flag && nr_flag) {
      fprintf(stderr, "--print-pairs and --non-redundant are incompatible\n\n");
      say_usage();
      return 1;
   }

   if (pp_flag && sp_flag) {
      fprintf(stderr, "--print-pairs and --spheres are incompatible\n\n");
      say_usage();
      return 1;
   }

   if (nr_flag && sp_flag) {
      fprintf(stderr, "--non-redundant and --spheres are incompatible\n\n");
      say_usage();
      return 1;
   }

   int output_type;

        if (pp_flag) output_type = PRINT_PAIRS;
   else if (nr_flag) output_type = PRINT_NRED;
   else if (sp_flag) output_type = SPHERES_OUTPUT;
   else              output_type = DEFAULT_OUTPUT;


   FILE *inputf;
   FILE *outputf;

   if (input != UNSET) {
      inputf = fopen(input, "r");
      if (inputf == NULL) {
         fprintf(stderr, "cannot open file %s\n\n", input);
         say_usage();
         return 1;
      }
   }
   else {
      inputf = stdin;
   }

   if (output != UNSET) {
      outputf = fopen(output, "w");
      if (outputf == NULL) {
         fprintf(stderr, "cannot write to file %s\n\n", output);
         say_usage();
         return 1;
      }
   }
   else {
      outputf = stdout;
   }

   // Set default options.
   if (dist < 0) dist = 3;
   if (threads < 0) threads = 1;

   int exitcode =
   starcode(
       inputf,
       outputf,
       dist,
       vb_flag,
       threads,
       output_type
   );

   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;

}
