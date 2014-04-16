#include "starcode.h"

char *USAGE = "Usage:\n"
"  tquery [-v] [-d dist] [-i input] [-q query] [-o output]\n"
"    -v --verbose: verbose\n"
"    -d --dist: maximum Levenshtein distance\n"
"    -t --threads: number of concurrent threads (default 2)\n"
"    -l --level: number of subtries (default 3*threads)\n"
"    -i --index: index file\n"
"    -q --query: query file\n"
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
   signal(SIGSEGV, SIGSEGV_handler);

   // Unset flags (value -1).
   int verbose_flag = -1;
   int dist_flag = -1;
   int threads_flag = -1;
   int ntries_flag = -1;
   // Unset options (value 'UNSET').
   char _u; char *UNSET = &_u;
   char *index = UNSET;
   char *query = UNSET;
   char *output = UNSET;

   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"verbose", no_argument,       0, 'v'},
         {"help",    no_argument,       0, 'h'},
         {"dist",    required_argument, 0, 'd'},
         {"index",   required_argument, 0, 'i'},
         {"query",   required_argument, 0, 'q'},
         {"output",  required_argument, 0, 'o'},
         {0, 0, 0, 0}
      };

      c = getopt_long(argc, argv, "t:l:d:i:ho:q:v",
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
            if(threads_flag < 0) {
               threads_flag = atoi(optarg);
            }
            else {
               fprintf(stderr, "threads option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'l':
            if(ntries_flag < 0) {
               ntries_flag = atoi(optarg);
            }
            else {
               fprintf(stderr, "level option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'i':
            if (index == UNSET) {
               index = optarg;
            }
            else {
               fprintf(stderr, "index option set more than once\n");
               say_usage();
               return 1;
            }
            break;

         case 'q':
            if (query == UNSET) {
               query = optarg;
            }
            else {
               fprintf(stderr, "query option set more than once\n");
               say_usage();
               return 1;
            }
            break;

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
      fprintf(stderr, "too many options\n");
      say_usage();
      return 1;
   }

   if (index == UNSET) {
      fprintf(stderr, "no index file specified\n");
      say_usage();
      return 1;
   }

   if (query == UNSET) {
      fprintf(stderr, "no query file specified\n");
      say_usage();
      return 1;
   }

   FILE *indexf;
   FILE *queryf;
   FILE *outputf;

   indexf = fopen(index, "r");
   if (indexf == NULL) {
      fprintf(stderr, "cannot open index file %s\n", index);
      say_usage();
      return 1;
   }

   queryf = fopen(query, "r");
   if (queryf == NULL) {
      fprintf(stderr, "cannot open query file %s\n", index);
      say_usage();
      return 1;
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

   if (verbose_flag < 0) verbose_flag = 0;
   if (dist_flag < 0) dist_flag = 3;
   if (threads_flag < 0) threads_flag = 2;
   if (ntries_flag < 0) ntries_flag = 3*threads_flag;

   int exitcode = tquery(indexf, queryf, outputf, dist_flag, verbose_flag, threads_flag, ntries_flag); 

   fclose(indexf);
   fclose(queryf);
   if (outputf != stdout) fclose(outputf);

   return exitcode;

}
