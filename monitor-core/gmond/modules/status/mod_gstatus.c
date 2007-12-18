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

#include <metric.h>
#include <gm_mmn.h>
#include <stdio.h>
#include <stdlib.h>

#include "file.h"
#include "debug_msg.h"
#include "error.h"
#include "scoreboard.h"

#include <apr_strings.h>

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule gstatus_module;

static apr_array_header_t *metric_info = NULL;
static Ganglia_25metric gs_metric_info[] = 
{
    {0, NULL}
};

static int gs_metric_init (apr_pool_t *p)
{
    void *sbi = ganglia_scoreboard_iterator();
    char *name;
    int i = 0;

    metric_info = apr_array_make(p, 2, sizeof(Ganglia_25metric));

    while (sbi) {
        name = ganglia_scoreboard_next(&sbi);
        Ganglia_25metric *gmi = apr_array_push(metric_info);

        /* gmi->key will be automatically assigned by gmond */
        gmi->name = apr_pstrdup (p, name);
        gmi->tmax = 90;
        gmi->type = GANGLIA_VALUE_UNSIGNED_INT;
        gmi->units = apr_pstrdup(p, "count");
        gmi->slope = apr_pstrdup(p, "both");
        gmi->fmt = apr_pstrdup(p, "%u");
        gmi->msg_size = UDP_HEADER_SIZE+8;
        gmi->desc = apr_pstrdup(p, "Gmond status metric");        

        MMETRIC_INIT_METADATA(gmi,p);
        MMETRIC_ADD_METADATA(gmi,MGROUP,"gstatus");
    }


    /* Replace the empty static metric definition array with the
       dynamic array that we just created 
    */
    gstatus_module.metrics_info = (Ganglia_25metric *)metric_info->elts;
    return 0;
}

static void gs_metric_cleanup ( void )
{
}

static g_val_t gs_metric_handler ( int metric_index )
{
    g_val_t val;
    Ganglia_25metric *gmi = &(gstatus_module.metrics_info[metric_index]);

    val.int32 = ganglia_scoreboard_get(gmi->name);
    return val;
}

mmodule gstatus_module =
{
    STD_MMODULE_STUFF,
    gs_metric_init,
    gs_metric_cleanup,
    gs_metric_info,
    gs_metric_handler,
};
