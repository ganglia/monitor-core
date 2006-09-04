#include "libmetrics.h"

int  libmetrics_initialized = 0;

void
libmetrics_init( void )
{
  if(libmetrics_initialized)
    return;

  metric_init();
  libmetrics_initialized = 1;
}
