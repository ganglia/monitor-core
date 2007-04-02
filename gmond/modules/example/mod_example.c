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
#include <time.h>

//#include <apr_strings.h>

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule example_module;

#define METRIC_NAME "example_module"
#define METRIC_UNITS "s"
#define METRIC_SLOPE "both"
#define METRIC_FMT "%u"
#define METRIC_DESC "Example module metric (random numbers)"

/*XXX gmond needs to link dynamically with apr so that each
   of the modules can link to apr as well.  Then the modules
   should use the apr string functions to allocate memory
   for the metric info structure rather than using the 
   stack.  Each of the callbacks should also take the
   memory pool as a parameter. */

static int ex_metric_init ( void )
{
    srand(time(NULL)%99);
    return 0;
}

static void ex_metric_cleanup ( void )
{
}

static void ex_metric_info ( Ganglia_25metric *gmi )
{
    /* gmi->key will be automatically assigned by gmond */
    gmi->name = METRIC_NAME;
    gmi->tmax = 90;
    gmi->type = GANGLIA_VALUE_UNSIGNED_INT;
    gmi->units = METRIC_UNITS;
    gmi->slope = METRIC_SLOPE;
    gmi->fmt = METRIC_FMT;
    gmi->msg_size = UDP_HEADER_SIZE+8;
    gmi->desc = METRIC_DESC;

    return;
}

static g_val_t ex_metric_handler ( void )
{
    g_val_t val;
    val.int32 = rand()%99;
    return val;
}

mmodule example_module =
{
    STD_MMODULE_STUFF,
    ex_metric_init,
    ex_metric_cleanup,
    ex_metric_info,
    ex_metric_handler,
};
