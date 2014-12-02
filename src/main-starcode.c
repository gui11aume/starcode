/*
** Copyright 2014 Guillaume Filion, Eduard Valera Zorita and Pol Cusco.
**
** File authors:
**  Guillaume Filion     (guillaume.filion@gmail.com)
**  Eduard Valera Zorita (ezorita@mit.edu)
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
#include <string.h>
#include <unistd.h>
#include "starcode.h"

char *USAGE =
"\n"
"Usage:"
"  starcode [options]\n"
"\n"
"  general options:\n"
"    -v --version: display version and exit\n"
"    -z --verbose: verbose\n"
"    -d --dist: maximum Levenshtein distance (default auto)\n"
"    -t --threads: number of concurrent threads (default 1)\n"
"    -s --sphere: sphere clustering (default message passing)\n"
"\n"
"  input/output options (single file, default)\n"
"    -i --input: input file (default stdin)\n"
"    -o --output: output file (default stdout)\n"
"\n"
"  input options (paired-end fastq files)\n"
"    -1 --input1: input file 1\n"
"    -2 --input2: input file 2\n"
"\n"
"  output format options\n"
"       --non-redundant: remove redundant sequences\n";

void say_usage(void) { fprintf(stderr, "%s\n", USAGE); }
void say_version(void) { fprintf(stderr, VERSION "\n"); }
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


char *
outname
(
   char *path
)
{

   static char name[320] = {0};
   if (strlen(path) > 310) {
      fprintf(stderr, "input file name too long (%s)\n", path);
      abort();
   }

   // Find final dot, append "-starcode" just before.
   // If no final dot, just append starcode as suffix.
   char *c = strrchr(path, '.');
   if (c == NULL) {
      sprintf(name, "%s-starcode", path);
   }
   else {
      *c = '\0';
      sprintf(name, "%s-starcode.%s", path, c+1);
      *c = '.';
   }

   return (char *) name;

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
   static int nr_flag = 0;
   static int sp_flag = 0;
   static int vb_flag = 0;

   // Unset flags (value -1).
   int dist = -1;
   int threads = -1;

   // Unset options (value 'UNSET').
   char * const UNSET = "unset";
   char * input   = UNSET;
   char * input1  = UNSET;
   char * input2  = UNSET;
   char * output  = UNSET;

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"non-redundant",     no_argument,       &nr_flag,  1 },
         {"sphere-clustering", no_argument,       &sp_flag,  1 },
         {"version",           no_argument,       &vb_flag, 'v'},
         {"verbose",           no_argument,       &vb_flag, 'z'},
         {"dist",              required_argument,        0, 'd'},
         {"help",              no_argument,              0, 'h'},
         {"input",             required_argument,        0, 'i'},
         {"input1",            required_argument,        0, '1'},
         {"input2",            required_argument,        0, '2'},
         {"output",            required_argument,        0, 'o'},
         {"threads",           required_argument,        0, 't'},
         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "1:2:d:hi:o:st:vz",
            long_options, &option_index);
 
      // Done parsing options? //
      if (c == -1) break;

      switch (c) {
      case 0:
         // A flag was set. //
         break;

      case '1':
         if (input1 == UNSET) {
            input1 = optarg;
         }
         else {
            fprintf(stderr, "error: --input1 set more than once\n");
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case '2':
         if (input2 == UNSET) {
            input2 = optarg;
         }
         else {
            fprintf(stderr, "error: --input2 set more than once\n");
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'd':
         if (dist < 0) {
            dist = atoi(optarg);
            if (dist > STARCODE_MAX_TAU) {
               fprintf(stderr, "error: --dist cannot exceed %d\n",
                     STARCODE_MAX_TAU);
               return EXIT_FAILURE;
            }
         }
         else {
            fprintf(stderr, "error: --distance set more than once\n");
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'h':
         // User asked for help. //
         say_version();
         say_usage();
         return 0;

      case 'i':
         if (input == UNSET) {
            input = optarg;
         }
         else {
            fprintf(stderr, "error: --input set more than once\n");
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'o':
         if (output == UNSET) {
            output = optarg;
         }
         else {
            fprintf(stderr, "error: --output set more than once\n");
            say_usage();
            return EXIT_FAILURE;
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
            fprintf(stderr, "error: --thread set more than once\n");
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'z':
         vb_flag = 1;
         break;

      case 'v':
         say_version();
         return EXIT_SUCCESS;

      default:
         // Cannot parse. //
         say_version();
         say_usage();
         return EXIT_FAILURE;

      }

   }

   if (optind < argc) {
      // If no input is specified, assume first positional argument
      // is the name of the input file.
      if ((optind == argc-1) && (input == UNSET && input1 == UNSET)) {
         input = argv[optind];
      }
      else {
         fprintf(stderr, "too many options\n");
         say_usage();
         return EXIT_FAILURE;
      }
   }

   // Check options compatibility. //
   if (nr_flag && sp_flag) {
      fprintf(stderr, "error: --non-redundant and --spheres are incompatible\n");
      say_usage();
      return EXIT_FAILURE;
   }
   if (input != UNSET && (input1 != UNSET || input2 != UNSET)) {
      fprintf(stderr, "error: --input and --input1/2 are incompatible\n");
      say_usage();
      return EXIT_FAILURE;
   }
   if (input1 == UNSET && input2 != UNSET) {
      fprintf(stderr, "error: --input1 set without --input2\n");
      say_usage();
      return EXIT_FAILURE;
   }
   if (input2 == UNSET && input1 != UNSET) {
      fprintf(stderr, "error: --input2 set without --input1\n");
      say_usage();
      return EXIT_FAILURE;
   }
   if (nr_flag && output != UNSET &&
         (input1 != UNSET || input2 != UNSET)) {
      fprintf(stderr, "error: cannot specify --output for paired-end "
            "fastq file with --non-redundant\n");
      say_usage();
      return EXIT_FAILURE;
   }

   // Set output type. //
   int output_type;
        if (nr_flag) output_type = PRINT_NRED;
   else if (sp_flag) output_type = SPHERES_OUTPUT;
   else              output_type = DEFAULT_OUTPUT;

   // Set input file(s). //
   FILE *inputf1 = NULL;
   FILE *inputf2 = NULL;

   // Set output file(s). //
   FILE *outputf1 = NULL;
   FILE *outputf2 = NULL;

   if (input != UNSET) {
      inputf1 = fopen(input, "r");
      if (inputf1 == NULL) {
         fprintf(stderr, "error: cannot open file %s\n", input);
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else if (input1 != UNSET) {
      inputf1 = fopen(input1, "r");
      if (inputf1 == NULL) {
         fprintf(stderr, "error: cannot open file %s\n", input1);
         say_usage();
         return EXIT_FAILURE;
      }
      inputf2 = fopen(input2, "r");
      if (inputf2 == NULL) {
         fprintf(stderr, "error: cannot open file %s\n", input2);
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else {
      inputf1 = stdin;
   }

   if (output != UNSET) {
      outputf1 = fopen(output, "w");
      if (outputf1 == NULL) {
         fprintf(stderr, "error: cannot write to file %s\n", output);
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else if (nr_flag && input1 != UNSET && input2 != UNSET) {
      outputf1 = fopen(outname(input1), "w");
      if (outputf1 == NULL) {
         fprintf(stderr, "error: cannot write to file %s\n", outname(input1));
         say_usage();
         return EXIT_FAILURE;
      }
      outputf2 = fopen(outname(input2), "w");
      if (outputf2 == NULL) {
         fprintf(stderr, "error: cannot write to file %s\n", outname(input2));
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else {
      outputf1 = stdout;
   }

   // Set default options.
   if (threads < 0) threads = 1;

   int exitcode =
   starcode(
       inputf1,
       inputf2,
       outputf1,
       outputf2,
       dist,
       vb_flag,
       threads,
       output_type
   );

   if (inputf1 != stdin)   fclose(inputf1);
   if (inputf2 != NULL)    fclose(inputf2);
   if (outputf1 != stdout) fclose(outputf1);
   if (outputf2 != NULL)   fclose(outputf2);

   return exitcode;

}
