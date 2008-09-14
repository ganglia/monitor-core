#include <stdio.h>
#include <unistd.h>
#include "libmetrics.h"
#include "config.h"

#ifdef MINGW
#include <windows.h>
#define sleep(s) Sleep(s*1000)
#endif

#define NUM_TESTS 3 
#define SLEEP_TIME 5

static const struct metricinfo
{
  const char *name;
    g_val_t (*func) (void);
  g_type_t type;
} metrics[] =
{
  {
  "cpu_num", cpu_num_func, g_uint16},
  {
  "cpu_speed", cpu_speed_func, g_uint32},
  {
  "mem_total", mem_total_func, g_float},
  {
  "swap_total", swap_total_func, g_float},
  {
  "boottime", boottime_func, g_timestamp},
  {
  "sys_clock", sys_clock_func, g_timestamp},
  {
  "machine_type", machine_type_func, g_string},
  {
  "os_name", os_name_func, g_string},
  {
  "os_release", os_release_func, g_string},
  {
  "cpu_user", cpu_user_func, g_float},
  {
  "cpu_nice", cpu_nice_func, g_float},
  {
  "cpu_system", cpu_system_func, g_float},
  {
  "cpu_idle", cpu_idle_func, g_float},
  {
  "cpu_wio", cpu_wio_func, g_float},
  {
  "cpu_aidle", cpu_aidle_func, g_float},
  {
  "load_one", load_one_func, g_float},
  {
  "load_five", load_five_func, g_float},
  {
  "load_fifteen", load_fifteen_func, g_float},
  {
  "proc_run", proc_run_func, g_uint32},
  {
  "proc_total", proc_total_func, g_uint32},
  {
  "mem_free", mem_free_func, g_float},
  {
  "mem_shared", mem_shared_func, g_float},
  {
  "mem_buffers", mem_buffers_func, g_float},
  {
  "mem_cached", mem_cached_func, g_float},
  {
  "swap_free", swap_free_func, g_float},
  {
  "mtu", mtu_func, g_uint32},
  {
  "bytes_out", bytes_out_func, g_float},
  {
  "bytes_in", bytes_in_func, g_float},
  {
  "pkts_in", pkts_in_func, g_float},
  {
  "pkts_out", pkts_out_func, g_float},
  {
  "disk_free", disk_free_func, g_double},
  {
  "disk_total", disk_total_func, g_double},
  {
  "part_max_used", part_max_used_func, g_float},
  {
  "", NULL}
};

int
main (void)
{
  g_val_t val;
  int check, i;

  /* Initialize libmetrics */
  metric_init ();

  /* Loop through a few times to make sure everything works */
  for (check = 0; check < NUM_TESTS; check++)
    {
      fprintf(stderr,"============= Running test #%d of %d =================\n", check+1, NUM_TESTS);

      /* Run through the metric list */
      for (i = 0; metrics[i].func != NULL; i++)
	{
	  fprintf (stderr, "%20s = ", metrics[i].name);
	  val = metrics[i].func ();
#if 0
	  if (!val)
	    {
	      fprintf (stderr, "NOT IMPLEMENTED ON THIS PLATFORM");
	    }
	  else
#endif
	    {
	      switch (metrics[i].type)
		{
		case g_string:
		  fprintf (stderr, "%s (g_string)", val.str);
		  break;
		case g_int8:
		  fprintf (stderr, "%d (g_int8)", (int) val.int8);
		  break;
		case g_uint8:
		  fprintf (stderr, "%d (g_uint8)", (unsigned int) val.uint8);
		  break;
		case g_int16:
		  fprintf (stderr, "%d (g_int16)", (int) val.int16);
		  break;
		case g_uint16:
		  fprintf (stderr, "%d (g_uint16)",
			   (unsigned int) val.uint16);
		  break;
		case g_int32:
		  fprintf (stderr, "%d (g_int32)", (int) val.int32);
		  break;
		case g_uint32:
		  fprintf (stderr, "%u (g_uint32)", (unsigned int)val.uint32);
		  break;
		case g_float:
		  fprintf (stderr, "%f (g_float)", val.f);
		  break;
		case g_double:
		  fprintf (stderr, "%f (g_double)", val.d);
		  break;
		case g_timestamp:
		  fprintf (stderr, "%u (g_timestamp)", (unsigned)val.uint32);
		  break;
		}
	    }
	  fprintf (stderr, "\n");
	}

      if(check != (NUM_TESTS - 1))
	sleep(SLEEP_TIME);
    }

  return 0;
}
