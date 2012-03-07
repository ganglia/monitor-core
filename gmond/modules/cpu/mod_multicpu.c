/*******************************************************************************
* Portions Copyright (C) 2007 Novell, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
*  - Neither the name of Novell, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
* Author: Brad Nicholes (bnicholes novell.com)
******************************************************************************/

#include <gm_metric.h>

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "gm_file.h"
#include "libmetrics.h"

#include <apr_tables.h>
#include <apr_strings.h>

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule multicpu_module;

static timely_file proc_stat    = { {0,0} , 1., "/proc/stat", NULL, BUFFSIZE };

struct cpu_util {
   g_val_t val;
   struct timeval stamp;
   double last_jiffies;
   double curr_jiffies;
   double last_total_jiffies;
   double curr_total_jiffies;
   double diff;
};
typedef struct cpu_util cpu_util;


#define NUM_CPUSTATES_24X 4
#define NUM_CPUSTATES_26X 7
static unsigned int num_cpustates;
static unsigned int cpu_count = 0;
static apr_pool_t *pool;
static cpu_util *cpu_user = NULL;
static cpu_util *cpu_nice = NULL;
static cpu_util *cpu_system = NULL;
static cpu_util *cpu_idle = NULL;
static cpu_util *cpu_wio = NULL;
static cpu_util *cpu_intr = NULL;
static cpu_util *cpu_sintr = NULL;

/*
 * A helper function to determine the number of cpustates in /proc/stat (MKN)
 * Count how many cpus there are in the system
 */
static void init_cpu_info (void)
{
    char *p;
    unsigned int i=0;
    
    proc_stat.last_read.tv_sec=0;
    proc_stat.last_read.tv_usec=0;
    p = update_file(&proc_stat);
    proc_stat.last_read.tv_sec=0;
    proc_stat.last_read.tv_usec=0;
    
    /*
    ** Skip initial "cpu" token
    */
    p = skip_token(p);
    p = skip_whitespace(p);

    /*
    ** Loop over file until next "cpu" token is found.
    ** i=4 : Linux 2.4.x
    ** i=7 : Linux 2.6.x
    */
    while (strncmp(p,"cpu",3)) {
        p = skip_token(p);
        p = skip_whitespace(p);
        i++;
    }
    num_cpustates = i;

    for (i=1; strlen(p) > 0;) {
        p = skip_token(p);
        p = skip_whitespace(p);
        if (strncmp(p,"cpu",3) == 0) {
            i++;
        }
    }
    cpu_count = i;
    
    return;
}

/*
 * Get the metric name and index from the original
 * metric name
 */
static void get_metric_name_cpu (char *metric, char *name, int *index)
{
    /* The metric name contains both the name and the cpu id. 
       Split the cpu id from the name so that we can use it to
       decide which metric handler to call and for which cpu.
    */
    size_t numIndex = strcspn (metric, "0123456789");

    if (numIndex > 0) {
        strncpy (name, metric, numIndex);
        name[numIndex] = '\0';
        *index = atoi(&metric[numIndex]);
    } else {
        *name = '\0';
        *index = 0;
    }

    return;
}

static double total_jiffies_func (char *p);

/*
 * Find the correct cpu info with the buffer,
 * given a cpu index
 */
static char *find_cpu (char *p, int cpu_index, double *total_jiffies)
{
    int i;

    /*
    ** Skip initial "cpu" token
    */
    p = skip_token(p);
    p = skip_whitespace(p);

    for (i=0; i<=cpu_index; i++) { 
        while (strlen(p) > 0) {
            p = skip_token(p);
            p = skip_whitespace(p);
            if (strncmp(p,"cpu",3) == 0) {
                break;
            }
        }
    }

    /*
    ** Skip last "cpu" token
    */
    p = skip_token(p);
    p = skip_whitespace(p);

    *total_jiffies = total_jiffies_func(p);

    return p;
}

/*
 * A helper function to return the total number of cpu jiffies
 * Assumes that "p" has been aligned already to the CPU line to evaluate
 */
static double total_jiffies_func (char *p)
{
    unsigned long user_jiffies, nice_jiffies, system_jiffies, idle_jiffies,
        wio_jiffies, irq_jiffies, sirq_jiffies;

    user_jiffies = strtod( p, &p );
    p = skip_whitespace(p);
    nice_jiffies = strtod( p, &p ); 
    p = skip_whitespace(p);
    system_jiffies = strtod( p , &p ); 
    p = skip_whitespace(p);
    idle_jiffies = strtod( p , &p );
    
    if (num_cpustates == NUM_CPUSTATES_24X)
        return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies;
    
    p = skip_whitespace(p);
    wio_jiffies = strtod( p , &p );
    p = skip_whitespace(p);
    irq_jiffies = strtod( p , &p );
    p = skip_whitespace(p);
    sirq_jiffies = strtod( p , &p );
    
    return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies +
        wio_jiffies + irq_jiffies + sirq_jiffies; 
}   

/* Common function to calculate the utilization */
static void calculate_utilization (char *p, cpu_util *cpu)
{
    cpu->curr_jiffies = strtod(p, (char **)NULL);

    cpu->diff = (cpu->curr_jiffies - cpu->last_jiffies); 

    if (cpu->diff)
        cpu->val.f = (cpu->diff/(cpu->curr_total_jiffies - cpu->last_total_jiffies))*100;
    else
        cpu->val.f = 0.0;

    cpu->last_jiffies = cpu->curr_jiffies;
    cpu->last_total_jiffies = cpu->curr_total_jiffies;

    return;
}

static g_val_t multi_cpu_user_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_user[cpu_index]);

    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        calculate_utilization (p, cpu);
    }

    return cpu->val;
}

static g_val_t multi_cpu_nice_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_nice[cpu_index]);
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_whitespace(p);

        calculate_utilization (p, cpu);
    }

    return cpu->val;
}

static g_val_t multi_cpu_system_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_system[cpu_index]);
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_whitespace(p);
        cpu->curr_jiffies = strtod(p , (char **)NULL);
        if (num_cpustates > NUM_CPUSTATES_24X) {
            p = skip_token(p);
            p = skip_token(p);
            p = skip_token(p);
            p = skip_whitespace(p);

            cpu->curr_jiffies += strtod(p , (char **)NULL); /* "intr" counted in system */
            p = skip_token(p);
            cpu->curr_jiffies += strtod(p , (char **)NULL); /* "sintr" counted in system */
        }
    
        cpu->diff = cpu->curr_jiffies - cpu->last_jiffies;
    
        if (cpu->diff)
            cpu->val.f = (cpu->diff/(cpu->curr_total_jiffies - cpu->last_total_jiffies))*100;
        else
            cpu->val.f = 0.0;
    
        cpu->last_jiffies = cpu->curr_jiffies;
        cpu->last_total_jiffies = cpu->curr_total_jiffies;   
    }

    return cpu->val;
}

static g_val_t multi_cpu_idle_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_idle[cpu_index]);
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_whitespace(p);
    
        calculate_utilization (p, cpu);
    }
    
    return cpu->val;
}

static g_val_t multi_cpu_wio_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_wio[cpu_index]);
    
    if (num_cpustates == NUM_CPUSTATES_24X) {
        cpu->val.f = 0.;
        return cpu->val;
    }
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_whitespace(p);
    
        calculate_utilization (p, cpu);
    }
    
    return cpu->val;
}

static g_val_t multi_cpu_intr_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_intr[cpu_index]);
    
    if (num_cpustates == NUM_CPUSTATES_24X) {
        cpu->val.f = 0.;
        return cpu->val;
    }
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_whitespace(p);
    
        calculate_utilization (p, cpu);
    }
    
    return cpu->val;
}

static g_val_t multi_cpu_sintr_func (int cpu_index)
{
    char *p;
    cpu_util *cpu = &(cpu_sintr[cpu_index]);
    
    if (num_cpustates == NUM_CPUSTATES_24X) {
        cpu->val.f = 0.;
        return cpu->val;
    }
    
    p = update_file(&proc_stat);
    if((proc_stat.last_read.tv_sec != cpu->stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != cpu->stamp.tv_usec)) {
        cpu->stamp = proc_stat.last_read;
    
        p = find_cpu (p, cpu_index, &cpu->curr_total_jiffies);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_token(p);
        p = skip_whitespace(p);
    
        calculate_utilization (p, cpu);
    }
    
    return cpu->val;
}

static apr_array_header_t *metric_info = NULL;

/* Initialize the give metric by allocating the per metric data
   structure and inserting a metric definition for each cpu found
*/
static cpu_util *init_metric (apr_pool_t *p, apr_array_header_t *ar, int cpu_count, char *name, char *desc)
{
    int i;
    Ganglia_25metric *gmi;
    cpu_util *cpu;

    cpu = apr_pcalloc (p, sizeof(cpu_util)*cpu_count);

    for (i=0; i<cpu_count; i++) {
        gmi = apr_array_push(ar);

        /* gmi->key will be automatically assigned by gmond */
        gmi->name = apr_psprintf (p, "%s%d", name, i);
        gmi->tmax = 90;
        gmi->type = GANGLIA_VALUE_FLOAT;
        gmi->units = apr_pstrdup(p, "%");
        gmi->slope = apr_pstrdup(p, "both");
        gmi->fmt = apr_pstrdup(p, "%.1f");
        gmi->msg_size = UDP_HEADER_SIZE+8;
        gmi->desc = apr_pstrdup(p, desc);        
    }

    return cpu;
}

static int ex_metric_init (apr_pool_t *p)
{
    int i;
    Ganglia_25metric *gmi;

    init_cpu_info ();

    /* Allocate a pool that will be used by this module */
    apr_pool_create(&pool, p);

    metric_info = apr_array_make(pool, 2, sizeof(Ganglia_25metric));

    /* Initialize each metric */
    cpu_user = init_metric (pool, metric_info, cpu_count, "multicpu_user", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the user level");
    cpu_nice = init_metric (pool, metric_info, cpu_count, "multicpu_nice", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the nice level");
    cpu_system = init_metric (pool, metric_info, cpu_count, "multicpu_system", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the system level");
    cpu_idle = init_metric (pool, metric_info, cpu_count, "multicpu_idle", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the idle level");
    cpu_wio = init_metric (pool, metric_info, cpu_count, "multicpu_wio", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the wio level");
    cpu_intr = init_metric (pool, metric_info, cpu_count, "multicpu_intr", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the intr level");
    cpu_sintr = init_metric (pool, metric_info, cpu_count, "multicpu_sintr", 
                            "Percentage of CPU utilization that occurred while "
                            "executing at the sintr level");

    /* Add a terminator to the array and replace the empty static metric definition 
        array with the dynamic array that we just created 
    */
    gmi = apr_array_push(metric_info);
    memset (gmi, 0, sizeof(*gmi));

    multicpu_module.metrics_info = (Ganglia_25metric *)metric_info->elts;

    for (i = 0; multicpu_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(multicpu_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(multicpu_module.metrics_info[i]),MGROUP,"cpu");
    }

    return 0;
}

static void ex_metric_cleanup ( void )
{
}

static g_val_t ex_metric_handler ( int metric_index )
{
    g_val_t val;
    int index;
    char name[64];

    /* Get the metric name and cpu index from the combined
       name that was passed in
    */
    get_metric_name_cpu (multicpu_module.metrics_info[metric_index].name, name, &index);

    if (strcmp(name, "multicpu_user") == 0) 
        return multi_cpu_user_func(index);

    if (strcmp(name, "multicpu_nice") == 0) 
        return multi_cpu_nice_func(index);

    if (strcmp(name, "multicpu_system") == 0) 
        return multi_cpu_system_func(index);

    if (strcmp(name, "multicpu_idle") == 0) 
        return multi_cpu_idle_func(index);

    if (strcmp(name, "multicpu_wio") == 0) 
        return multi_cpu_wio_func(index);

    if (strcmp(name, "multicpu_intr") == 0) 
        return multi_cpu_intr_func(index);

    if (strcmp(name, "multicpu_sintr") == 0) 
        return multi_cpu_sintr_func(index);

    /* default case */
    val.f = 0;
    return val;
}

mmodule multicpu_module =
{
    STD_MMODULE_STUFF,
    ex_metric_init,
    ex_metric_cleanup,
    NULL, /* defined dynamically */
    ex_metric_handler,
};
