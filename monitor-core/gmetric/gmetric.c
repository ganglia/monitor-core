#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ganglia.h"
#include "cmdline.h"

Ganglia_gmetric gmetric;

/* The commandline options */
struct gengetopt_args_info args_info;

int
main( int argc, char *argv[] )
{
  /* process the commandline options */
  if (cmdline_parser (argc, argv, &args_info) != 0)
        exit(1);


  return 0;
}
