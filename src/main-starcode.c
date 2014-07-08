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

#include "starcode.h"

char *USAGE = "Usage:\n"
"  starcode [-i] input [-o output]\n"
"    -v --verbose: verbose\n"
"    -d --dist: maximum Levenshtein distance (default 3)\n"
"    -s --spheres: cluster as sphere instead of message passing\n"
"    -t --threads: number of concurrent threads (default 1)\n"
//"    -f --format: output 'rel' or 'counts' (default)\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)";

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

   // Unset flags (value -1).
   int verbose_flag = -1;
//   int format_flag = -1;
   int dist_flag = -1;
   int threads_flag = -1;
   int cluster_flag = -1;
   // Unset options (value 'UNSET').
   char * const UNSET = "unset";
//   char *format_option = UNSET;
   char *input = UNSET;
   char *output = UNSET;

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"verbose", no_argument,       0, 'v'},
         {"help",    no_argument,       0, 'h'},
         {"spheres", no_argument,       0, 's'},
//         {"format",  required_argument, 0, 'f'},
         {"dist",    required_argument, 0, 'd'},
         {"input",   required_argument, 0, 'i'},
         {"output",  required_argument, 0, 'o'},
         {"threads", required_argument, 0, 't'},
         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "d:i:f:t:ho:vs",
            long_options, &option_index);
 
      /* Detect the end of the options. */
      if (c == -1) break;
  
      switch (c) {
      case 'd':
         if (dist_flag < 0) {
            dist_flag = atoi(optarg);
         }
         else {
            fprintf(stderr, "distance option set more than once\n");
            say_usage();
            return 1;
         }
         break;

      case 't':
         if (threads_flag < 0) {
            threads_flag = atoi(optarg);
         }
         else {
            fprintf(stderr, "thread option set more than once\n");
            say_usage();
            return 1;
         }
         break;            

      case 'i':
         if (input == UNSET) {
            input = optarg;
         }
         else {
            fprintf(stderr, "input option set more than once\n");
            say_usage();
            return 1;
         }
         break;

// The current version of starcode has a single output format.
// Possible options would be to suppress the cluster composition
// in third column, or replace it by a compressed representation
// of the alignment (e.g. gem output).
/*
      case 'f':
         if (format_option == UNSET) {
            if (strcmp(optarg, "counts") == 0) {
               format_flag = 0;
            }
            else if (strcmp(optarg, "rel") == 0) {
               format_flag = 1;
            }
            else {
               fprintf(stderr, "invalid format option\n");
               say_usage();
               return 1;
            }
            format_option = optarg;
         }
         else {
            fprintf(stderr, "format option set more than once\n");
            say_usage();
            return 1;
         }
         break;
*/
  
      case 'o':
         if (output == UNSET) {
            output = optarg;
         }
         else {
            fprintf(stderr, "output option set more than once\n");
            say_usage();
            return 1;
         }
         break;

      case 's':
         if (cluster_flag < 0) {
            cluster_flag = 1;
         }
         else {
            fprintf(stderr, "spheres option set more than once\n");
            say_usage();
            return 1;
         }
         break;

      case 'v':
         if (verbose_flag < 0) {
            verbose_flag = 1;
         }
         else {
            fprintf(stderr, "verbose option set more than once\n");
            say_usage();
            return 1;
         }
         break;

      case 'h':
         say_usage();
         return 0;
 
      default:
         say_usage();
         return 1;

      }

   }

   if (optind < argc) {
      if ((optind == argc - 1) && (input == UNSET)) {
         input = argv[optind];
      }
      else {
         fprintf(stderr, "too many options\n");
         say_usage();
         return 1;
      }
   }

   FILE *inputf;
   FILE *outputf;

   if (input != UNSET) {
      inputf = fopen(input, "r");
      if (inputf == NULL) {
         fprintf(stderr, "cannot open file %s\n", input);
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
         fprintf(stderr, "cannot write to file %s\n", output);
         say_usage();
         return 1;
      }
   }
   else {
      outputf = stdout;
   }

   // Set default flags.
   if (verbose_flag < 0) verbose_flag = 0;
//   if (format_flag < 0) format_flag = 0;
   if (dist_flag < 0) dist_flag = 3;
   if (threads_flag < 0) threads_flag = 1;
   if (cluster_flag < 0) cluster_flag = 0;

   int exitcode = starcode(
                      inputf,
                      outputf,
                      dist_flag,
//                      format_flag,
                      verbose_flag,
                      threads_flag,
                      cluster_flag
                  );

   if (inputf != stdin) fclose(inputf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;

}
