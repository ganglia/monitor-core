#include <stdio.h>
#include "conf.h"

extern cfg_t *config_file;

/* This function is necessary only because I need
 * to know if the user set metric thresholds */
int metric_validate_func(cfg_t *cfg, cfg_opt_t *opt)
{
  char buf[1024];
  snprintf(buf, 1024,"%s_given", opt->name);
  cfg_setbool(cfg, buf, 1);
  return 0;
}

void 
init_validate_funcs(void)
{
  /* This is annoying but necessary.  I need to know if the value (a float) was set by the user. */
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_minimum", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_minimum_alert", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_minimum_warning", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_maximum_warning", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_maximum_alert", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|absolute_maximum", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|relative_change_normal", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|relative_change_warning", metric_validate_func);
  cfg_set_validate_func( config_file, "collection_group|metric|relative_change_alert", metric_validate_func);
}

int
value_callback(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
  fprintf(stderr,"CALLED\n");
  return 0;
}
