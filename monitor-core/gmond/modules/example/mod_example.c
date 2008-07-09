/*******************************************************************************
* Copyright (C) 2007 Novell, Inc. All rights reserved.
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

/*
 * The ganglia metric "C" interface, required for building DSO modules.
 */
#include <gm_metric.h>

#include <stdlib.h>
#include <strings.h>
#include <time.h>

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
extern mmodule example_module;

static unsigned int random_max = 50;
static unsigned int constant_value = 20;

static int ex_metric_init ( apr_pool_t *p )
{
    const char* str_params = example_module.module_params;
    apr_array_header_t *list_params = example_module.module_params_list;
    mmparam *params;
    int i;

    srand(time(NULL)%99);

    /* Read the parameters from the gmond.conf file. */
    /* Single raw string parameter */
    if (str_params) {
        debug_msg("[mod_example]Received string params: %s", str_params);
    }
    /* Multiple name/value pair parameters. */
    if (list_params) {
        debug_msg("[mod_example]Received following params list: ");
        params = (mmparam*) list_params->elts;
        for(i=0; i< list_params->nelts; i++) {
            debug_msg("\tParam: %s = %s", params[i].name, params[i].value);
            if (!strcasecmp(params[i].name, "RandomMax")) {
                random_max = atoi(params[i].value);
            }
            if (!strcasecmp(params[i].name, "ConstantValue")) {
                constant_value = atoi(params[i].value);
            }
        }
    }

    /* Initialize the metadata storage for each of the metrics and then
     *  store one or more key/value pairs.  The define MGROUPS defines
     *  the key for the grouping attribute. */
    MMETRIC_INIT_METADATA(&(example_module.metrics_info[0]),p);
    MMETRIC_ADD_METADATA(&(example_module.metrics_info[0]),MGROUP,"random");
    MMETRIC_ADD_METADATA(&(example_module.metrics_info[0]),MGROUP,"example");
    /*
     * Usually a metric will be part of one group, but you can add more
     * if needed as shown above where Random_Numbers is both in the random
     * and example groups.
     */

    MMETRIC_INIT_METADATA(&(example_module.metrics_info[1]),p);
    MMETRIC_ADD_METADATA(&(example_module.metrics_info[1]),MGROUP,"example");

    return 0;
}

static void ex_metric_cleanup ( void )
{
}

static g_val_t ex_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        val.uint32 = rand()%random_max;
        break;
    case 1:
        val.uint32 = constant_value;
        break;
    default:
        val.uint32 = 0; /* default fallback */
    }

    return val;
}

static Ganglia_25metric ex_metric_info[] = 
{
    {0, "Random_Numbers", 90, GANGLIA_VALUE_UNSIGNED_INT, "Num", "both", "%u", UDP_HEADER_SIZE+8, "Example module metric (random numbers)"},
    {0, "Constant_Number", 90, GANGLIA_VALUE_UNSIGNED_INT, "Num", "zero", "%u", UDP_HEADER_SIZE+8, "Example module metric (constant number)"},
    {0, NULL}
};

mmodule example_module =
{
    STD_MMODULE_STUFF,
    ex_metric_init,
    ex_metric_cleanup,
    ex_metric_info,
    ex_metric_handler,
};
