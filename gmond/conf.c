#include <stdio.h>
#include "conf.h"


/* This function is necessary only because I need
 * to know if the user set metric thresholds */
int metric_validate_func(cfg_t *cfg, cfg_opt_t *opt)
{
  char buf[1024];
  snprintf(buf, 1024,"%s_given", opt->name);
  cfg_setbool(cfg, buf, 1);
  return 0;
}
