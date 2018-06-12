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

#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "starcode.h"
#include "trie.h"

// Prototypes for utilities of the main.
void   say_version (void);
void   SIGSEGV_handler (int);

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



int
main(
   int argc,
   char **argv
)
{
   // Backtrace handler
   signal(SIGSEGV, SIGSEGV_handler); 

   // Set flags to defaults.
   static int nr_flag = 0;
   static int sp_flag = 0;
   static int vb_flag = 1;
   static int cl_flag = 0;
   static int id_flag = 0;
   static int cp_flag = 0;

   // Unset flags (value -1).
   int dist = -1;
   int threads = -1;
   int cluster_ratio = -1;

   // Unset options (value 'UNSET').
   char * const UNSET = "unset";
   char * input   = UNSET;
   char * input1  = UNSET;
   char * input2  = UNSET;
   char * output  = UNSET;
   char * output1 = UNSET;
   char * output2 = UNSET;


   if (argc == 1 && isatty(0)) {
      say_usage();
      return EXIT_SUCCESS;
   }

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"print-clusters",    no_argument,       &cl_flag,  1 },
         {"seq-id",            no_argument,       &id_flag,  1 },
         {"non-redundant",     no_argument,       &nr_flag,  1 },
         {"quiet",             no_argument,       &vb_flag,  0 },
         {"sphere",            no_argument,       &sp_flag, 's'},
         {"connected-comp",    no_argument,       &cp_flag, 'c'},
         {"version",           no_argument,              0, 'v'},
         {"dist",              required_argument,        0, 'd'},
         {"cluster-ratio",     required_argument,        0, 'r'},
         {"help",              no_argument,              0, 'h'},
         {"input",             required_argument,        0, 'i'},
         {"input1",            required_argument,        0, '1'},
         {"input2",            required_argument,        0, '2'},
         {"output",            required_argument,        0, 'o'},
         {"threads",           required_argument,        0, 't'},
         {"output1",           required_argument,        0, '3'},
         {"output2",           required_argument,        0, '4'},

         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "1:2:3:4:d:hi:o:qcst:r:v",
            long_options, &option_index);
 
      // Done parsing //
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
            fprintf(stderr, "%s --input1 set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case '2':
         if (input2 == UNSET) {
            input2 = optarg;
         }
         else {
            fprintf(stderr, "%s --input2 set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case '3':
         if (output1 == UNSET) {
            output1 = optarg;
         }
         else {
            fprintf(stderr, "%s --output1 set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case '4':
         if (output2 == UNSET) {
            output2 = optarg;
         }
         else {
            fprintf(stderr, "%s --output2 set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;


      case 'd':
         if (dist < 0) {
            dist = atoi(optarg);
            if (dist > STARCODE_MAX_TAU) {
               fprintf(stderr, "%s --dist cannot exceed %d\n",
                     ERRM, STARCODE_MAX_TAU);
               return EXIT_FAILURE;
            }
         }
         else {
            fprintf(stderr, "%s --distance set more than once\n", ERRM);
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
            fprintf(stderr, "%s --input set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'o':
         if (output == UNSET) {
            output = optarg;
         }
         else {
            fprintf(stderr, "%s --output set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'q':
         vb_flag = 0;
         break;

      case 's':
         sp_flag = 1;
         break;

      case 'c':
         cp_flag = 1;
         break;

      case 't':
         if (threads < 0) {
            threads = atoi(optarg);
            if (threads < 1) {
               fprintf(stderr,
                     "%s --threads must be a positive " "integer\n", ERRM);
               say_usage();
               return EXIT_FAILURE;
            }
         }
         else {
            fprintf(stderr, "%s --thread set more than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'r':
         if (cluster_ratio < 0) {
            cluster_ratio = atoi(optarg);
            if (cluster_ratio < 1) {
               fprintf(stderr, "%s --cluster-ratio must be "
                     "a positive integer\n", ERRM);
               say_usage();
               return EXIT_FAILURE;
            }
         }
         else {
            fprintf(stderr,
                  "%s --cluster-ratio set more " "than once\n", ERRM);
            say_usage();
            return EXIT_FAILURE;
         }
         break;

      case 'v':
         say_version();
         return EXIT_SUCCESS;

      default:
         // Cannot parse. //
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
         fprintf(stderr, "%s too many options\n", ERRM);
         say_usage();
         return EXIT_FAILURE;
      }
   }

   // set default input and check flag compatibility
   input_compatibility_t ic = check_input (nr_flag,cl_flag,id_flag,sp_flag,cp_flag,
       &threads,&cluster_ratio,input1,input2,input,output);
   if (ic != INPUT_OK) return EXIT_FAILURE;


   // Set output type. //
   int output_type;
   if      (nr_flag) output_type = NRED_OUTPUT;
   else              output_type = DEFAULT_OUTPUT;

   int cluster_alg;
   if      (cp_flag) cluster_alg = COMPONENTS_CLUSTER;
   else if (sp_flag) cluster_alg = SPHERES_CLUSTER;
   else              cluster_alg = MP_CLUSTER;



   // Set input file(s). //
   FILE *inputf1 = NULL;
   FILE *inputf2 = NULL;

   // Set output file(s). //
   FILE *outputf1 = NULL;
   FILE *outputf2 = NULL;

   if (input != UNSET) {
      inputf1 = fopen(input, "r");
      if (inputf1 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input);
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else if (input1 != UNSET) {
      inputf1 = fopen(input1, "r");
      if (inputf1 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input1);
         say_usage();
         return EXIT_FAILURE;
      }
      inputf2 = fopen(input2, "r");
      if (inputf2 == NULL) {
         fprintf(stderr, "%s cannot open file %s\n", ERRM, input2);
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
         fprintf(stderr, "%s cannot write to file %s\n", ERRM, output);
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else if (nr_flag && input1 != UNSET && input2 != UNSET) {
      // Set default names as inputX-starcode.fastq
      if (output1 == UNSET) {
         output1 = outname(input1);
         outputf1 = fopen(output1, "w");
         free(output1);
      } else {
         outputf1 = fopen(output1, "w");
      }

      if (outputf1 == NULL) {
         fprintf(stderr,
               "%s cannot write to file %s\n", ERRM, outname(input1));
         say_usage();
         return EXIT_FAILURE;
      }

      if (output2 == UNSET) {
         output2 = outname(input2);
         outputf2 = fopen(output2, "w");
         free(output2);
      } else {
         outputf2 = fopen(output2, "w");
      }

      if (outputf2 == NULL) {
         fprintf(stderr,
               "%s cannot write to file %s\n", ERRM, outname(input2));
         say_usage();
         return EXIT_FAILURE;
      }
   }
   else {
      outputf1 = stdout;
   }

   if (vb_flag) {
      fprintf(stderr, "running starcode with %d thread%s\n",
           threads, threads > 1 ? "s" : "");
      fprintf(stderr, "reading input files\n");
   }
   gstack_t *uSQ = read_file(inputf1, inputf2, vb_flag);
   if (uSQ == NULL || uSQ->nitems < 1) {
      fprintf(stderr, "input file empty\n");
      return 1;
   }

   int exitcode =
   starcode(
       uSQ,
       outputf1,
       outputf2,
       dist,
       vb_flag,
       threads,
       cluster_alg,
       cluster_ratio,
       cl_flag,
       id_flag,
       output_type
   );

   if (inputf1 != stdin)   fclose(inputf1);
   if (inputf2 != NULL)    fclose(inputf2);
   if (outputf1 != stdout) fclose(outputf1);
   if (outputf2 != NULL)   fclose(outputf2);

   return exitcode;

}
