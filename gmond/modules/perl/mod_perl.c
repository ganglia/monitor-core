/*******************************************************************************
*
* Copyright (C) 2010 Bernard Li <bernard@vanhpc.org>
*               All Rights Reserved.
*
* Borrowed heavily from gmond/modules/mod_python.c
*
* This code is part of a perl module for ganglia.
*
* Contributors : Nicolas Brousse <nicolas@brousse.info>
*
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
******************************************************************************/

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include <gm_metric.h>
#include <gm_msg.h>
#include <string.h>

#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_lib.h>
#include <dirent.h>

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule perl_module;

typedef struct
{
    char *pcb;          	/* The metric call back function */
    char *mod_name;     	/* Name of the module */
    PerlInterpreter *perl; 	/* Perl interpreter */
}
mapped_info_t;

typedef struct
{
    char mname[128];
    int tmax;
    char vtype[32];
    char units[64];
    char slope[32];
    char format[64];
    char desc[512];
    char groups[512];
    apr_table_t *extra_data;
    char pcb[128];
}
pl_metric_init_t;

static apr_pool_t *pool;

static apr_array_header_t *metric_info = NULL;
static apr_array_header_t *metric_mapping_info = NULL;
static apr_status_t perl_metric_cleanup ( void *data);

char modname_bfr[PATH_MAX];
static char* is_perl_module(const char* fname)
{
    char* p = strrchr(fname, '.');
    if (!p) {
        return NULL;
    }

    if (strcmp(p, ".pl")) {
        return NULL;
    }

    strncpy(modname_bfr, fname, p-fname);
    modname_bfr[p-fname] = 0;
    return modname_bfr;
}

static cfg_t* find_module_config(char *modname)
{
    cfg_t *modules_cfg;
    int j;

    modules_cfg = cfg_getsec(perl_module.config_file, "modules");
    for (j = 0; j < cfg_size(modules_cfg, "module"); j++) {
        char *modName, *modLanguage;
        int modEnabled;

        cfg_t *plmodule = cfg_getnsec(modules_cfg, "module", j);

        /* Check the module language to make sure that
           the language designation is perl. */
        modLanguage = cfg_getstr(plmodule, "language");
        if (!modLanguage || strcasecmp(modLanguage, "perl"))
            continue;

        modName = cfg_getstr(plmodule, "name");
        if (strcasecmp(modname, modName))
            continue;

        /* Check to make sure that the module is enabled. */
        modEnabled = cfg_getbool(plmodule, "enabled");
        if (!modEnabled)
            continue;

        return plmodule;
    }
    return NULL;
}

static HV* build_params_hash(cfg_t *plmodule)
{
    int k;
    HV *params_hash;

    params_hash = newHV();

    if (plmodule && params_hash) {
        for (k = 0; k < cfg_size(plmodule, "param"); k++) {
            cfg_t *param;
            char *name, *value;
            SV *sv_value;

            param = cfg_getnsec(plmodule, "param", k);
            name = apr_pstrdup(pool, param->title);
            value = apr_pstrdup(pool, cfg_getstr(param, "value"));
            sv_value = newSVpv(value, 0);
            if (name && sv_value) {
                /* Silence "value computed is not used" warning */
                (void)hv_store(params_hash, name, strlen(name), sv_value, 0);
            }
        }
    }
    return params_hash;
} 

static void fill_metric_info(HV* plhash, pl_metric_init_t* minfo, char *modname, apr_pool_t *pool)
{
    char *metric_name = "";
    char *key;
    SV* sv_value;
    I32 ret;

    memset(minfo, 0, sizeof(*minfo));

    /* create the apr table here */
    minfo->extra_data = apr_table_make(pool, 2);

    hv_iterinit(plhash);
    while ((sv_value = hv_iternextsv(plhash, &key, &ret))) {
        if (!strcasecmp(key, "name")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->mname, value, sizeof(minfo->mname))) {
                err_msg("[PERL] No metric name given in module [%s].\n", modname);
            }
            else
                metric_name = minfo->mname;
            continue;
        }

        if (!strcasecmp(key, "call_back")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->pcb, value, sizeof(minfo->pcb))) {
                err_msg("[PERL] No perl call back given for metric [%s] in module [%s]. Will not call\n",
                        metric_name, modname);
            }
            continue;
        }
        if (!strcasecmp(key, "time_max")) {
            int value = SvIV(sv_value);
            if (!(minfo->tmax = value)) {
                minfo->tmax = 60;
                err_msg("[PERL] No time max given for metric [%s] in module [%s]. Using %d.\n",
                        metric_name, modname, minfo->tmax);
            }
            continue;
        }
        if (!strcasecmp(key, "value_type")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->vtype, value, sizeof(minfo->vtype))) {
                strcpy (minfo->vtype, "uint");
                err_msg("[PERL] No value type given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->vtype);
            }
            continue;
        }
        if (!strcasecmp(key, "units")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->units, value, sizeof(minfo->units))) {
                strcpy (minfo->units, "unknown");
                err_msg("[PERL] No metric units given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->units);
            }
            continue;
        }
        if (!strcasecmp(key, "slope")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->slope, value, sizeof(minfo->slope))) {
                strcpy (minfo->slope, "both");
                err_msg("[PERL] No slope given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->slope);
            }
            continue;
        }
        if (!strcasecmp(key, "format")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->format, value, sizeof(minfo->format))) {
                strcpy (minfo->format, "%u");
                err_msg("[PERL] No format given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->format);
            }
            continue;
        }
        if (!strcasecmp(key, "description")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->desc, value, sizeof(minfo->desc))) {
                strcpy (minfo->desc, "unknown metric");
                err_msg("[PERL] No description given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->desc);
            }
            continue;
        }
        if (!strcasecmp(key, "groups")) {
            STRLEN len;
            char *value = SvPV(sv_value, len);
            if (!strncpy(minfo->groups, value, sizeof(minfo->groups))) {
                strcpy (minfo->groups, "");
            }
            continue;
        }

        STRLEN len;
        char *value;
        if (!(value = SvPV(sv_value, len))) {
            err_msg("[PERL] Extra data key [%s] could not be processed.\n", key);
        }
        else {
            apr_table_add(minfo->extra_data, key, value);
        }        
    }
   
    /*err_msg("name: %s", minfo->mname);
    err_msg("callback: %s", minfo->pcb);
    err_msg("time_max: %d", minfo->tmax);
    err_msg("value_type: %s", minfo->vtype);
    err_msg("units: %s", minfo->units);
    err_msg("slope: %s", minfo->slope);
    err_msg("format: %s", minfo->format);
    err_msg("description: %s", minfo->desc);
    err_msg("groups: %s", minfo->groups);*/
}

static void fill_gmi(Ganglia_25metric* gmi, pl_metric_init_t* minfo)
{
    char *s, *lasts;
    int i;
    const apr_array_header_t *arr = apr_table_elts(minfo->extra_data);
    const apr_table_entry_t *elts = (const apr_table_entry_t *)arr->elts;

    /* gmi->key will be automatically assigned by gmond */
    gmi->name = apr_pstrdup(pool, minfo->mname);
    gmi->tmax = minfo->tmax;
    if (!strcasecmp(minfo->vtype, "string")) {
        gmi->type = GANGLIA_VALUE_STRING;
        gmi->msg_size = UDP_HEADER_SIZE+MAX_G_STRING_SIZE;
    }
    else if (!strcasecmp(minfo->vtype, "uint")) {
        gmi->type = GANGLIA_VALUE_UNSIGNED_INT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "int")) {
        gmi->type = GANGLIA_VALUE_INT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "float")) {
        gmi->type = GANGLIA_VALUE_FLOAT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "double")) {
        gmi->type = GANGLIA_VALUE_DOUBLE;
        gmi->msg_size = UDP_HEADER_SIZE+16;
    }
    else {
        gmi->type = GANGLIA_VALUE_UNKNOWN;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }

    gmi->units = apr_pstrdup(pool, minfo->units);
    gmi->slope = apr_pstrdup(pool, minfo->slope);
    gmi->fmt = apr_pstrdup(pool, minfo->format);
    gmi->desc = apr_pstrdup(pool, minfo->desc);

    MMETRIC_INIT_METADATA(gmi, pool);
    for (s=(char *)apr_strtok(minfo->groups, ",", &lasts);
          s!=NULL; s=(char *)apr_strtok(NULL, ",", &lasts)) {
        char *d = s;
        /* Strip the leading white space */
        while (d && *d && apr_isspace(*d)) {
            d++;
        }
        MMETRIC_ADD_METADATA(gmi,MGROUP,d);
    }

    /* transfer any extra data as metric metadata */
    for (i = 0; i < arr->nelts; ++i) {
        if (elts[i].key == NULL)
            continue;
        MMETRIC_ADD_METADATA(gmi, elts[i].key, elts[i].val);
    }
}

static int perl_metric_init(apr_pool_t *p)
{
    DIR *dp;
    struct dirent *entry;
    int i, size;
    char* modname;
    char *modpath;
    HV *pparamhash;
    pl_metric_init_t minfo;
    Ganglia_25metric *gmi;
    mapped_info_t *mi;
    const char* path = perl_module.module_params;
    cfg_t *module_cfg;
    PerlInterpreter *perl = NULL;
    int argc = 0;
	char *argv[] = { };
	char *env[] = { };
	char *embedding[] = {"", ""};

    /* Allocate a pool that will be used by this module */
    apr_pool_create(&pool, p);

    metric_info = apr_array_make(pool, 10, sizeof(Ganglia_25metric));
    metric_mapping_info = apr_array_make(pool, 10, sizeof(mapped_info_t));

    /* Verify path exists and can be read */

    if (!path) {
        err_msg("[PERL] Missing perl module path.\n");
        return -1;
    }

    if (access(path, F_OK)) {
        /* 'path' does not exist */
        err_msg("[PERL] Can't open the perl module path %s.\n", path);
        return -1;
    }

    if (access(path, R_OK)) {
        /* Don't have read access to 'path' */
        err_msg("[PERL] Can't read from the perl module path %s.\n", path);
        return -1;
    }

    /* Initialize each perl module */
    if ((dp = opendir(path)) == NULL) {
        /* Error: Cannot open the directory - Shouldn't happen */
        /* Log? */
        err_msg("[PERL] Can't open the perl module path %s.\n", path);
        return -1;
    }

    PERL_SYS_INIT3(&argc, (char ***) &argv, (char ***) &env);

    i = 0;

    while ((entry = readdir(dp)) != NULL) {
        modname = is_perl_module(entry->d_name);
        if (modname == NULL)
            continue;

        /* Find the specified module configuration in gmond.conf 
           If this return NULL then either the module config
           doesn't exist or the module is disabled. */
        module_cfg = find_module_config(modname);
        if (!module_cfg)
            continue;

        size_t path_len = strlen(path) + strlen(modname) + 5;
        modpath = malloc(path_len);
        modpath = strncpy(modpath, path, path_len);
        modpath = strcat(modpath, "/");
        modpath = strcat(modpath, modname);
        modpath = strcat(modpath, ".pl");

        embedding[1] = modpath ;

        perl = perl_alloc();
        PL_perl_destruct_level = 0;

        PERL_SET_CONTEXT(perl);
        perl_construct(perl);
        PL_origalen = 1;

        PERL_SET_CONTEXT(perl);
        perl_parse(perl, NULL, 1, embedding, NULL);

        /* Run the perl script so that global variables can be accessed */
        perl_run(perl);

        free(modpath);

        /* Build a parameter dictionary to pass to the module */
        pparamhash = build_params_hash(module_cfg);
        if (!pparamhash) {
            err_msg("[PERL] Can't build the parameters hash for [%s].\n", modname);
            continue;
        }

        dSP;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        /* Push a reference to the pparamhash to the Perl stack */
        XPUSHs(sv_2mortal(newRV_noinc((SV*)pparamhash)));
        PUTBACK;
        size = call_pv("metric_init", G_ARRAY|G_EVAL);
        SPAGAIN;
        /*SP -= size;
        ax = (SP - PL_stack_base) + 1;
        SvGETMAGIC(plarray); */

        if (SvTRUE(ERRSV)) {
            /* failed calling metric_init */
            err_msg("[PERL] Can't call the metric_init function in the perl module [%s].\n", modname);
            continue;
        }
        else {
            if (size) {
                int j;
                for (j = 0; j < size; j++) {
                    SV* sref = POPs;
                    if (!SvROK(sref)) {
                        err_msg("[PERL] No descriptors returned from metric_init call in the perl module [%s].\n", modname);
                        continue;
                    }

                    /* Dereference the reference */
                    HV* plhash = (HV*)(SvRV(sref));
                    if (plhash != NULL) {
                        fill_metric_info(plhash, &minfo, modname, pool);
                        gmi = (Ganglia_25metric*)apr_array_push(metric_info);
                        fill_gmi(gmi, &minfo);
                        mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
                        mi->mod_name = apr_pstrdup(pool, modname);
                        mi->pcb = apr_pstrdup(pool, minfo.pcb);
                        mi->perl = perl;
                    }
                }
            }
        }

        PUTBACK;
        FREETMPS;
        LEAVE;

    }
    closedir(dp);

    apr_pool_cleanup_register(pool, NULL,
                              perl_metric_cleanup,
                              apr_pool_cleanup_null);

    /* Replace the empty static metric definition array with the
       dynamic array that we just created 
     */
    /*XXX Need to put this into a finalize MACRO. This is just pushing
      a NULL entry onto the array so that the looping logic can 
      determine the end if the array. We should probably give back
      a ready APR array rather than a pointer to a Ganglia_25metric
      array. */
    gmi = apr_array_push(metric_info);
    memset (gmi, 0, sizeof(*gmi));
    mi = apr_array_push(metric_mapping_info);
    memset (mi, 0, sizeof(*mi));

    perl_module.metrics_info = (Ganglia_25metric *)metric_info->elts;
    return 0;
}

static apr_status_t perl_metric_cleanup(void *data)
{
    mapped_info_t *mi, *smi;
    int i, j;

    mi = (mapped_info_t*) metric_mapping_info->elts;
    for (i = 0; i < metric_mapping_info->nelts; i++) {
        if (mi[i].mod_name) {

        	/* XXX: Should work but segfault...
        	if (mi[i].perl != NULL) {
				PERL_SET_CONTEXT(mi[i].perl);
				perl_destruct(mi[i].perl);

				PERL_SET_CONTEXT(mi[i].perl);
				perl_free(mi[i].perl);
        	}
        	*/

            /* Set all modules that fall after this once with the same
             * module pointer to NULL so metric_cleanup only gets called
             * once on the module.
             */
            smi = (mapped_info_t*) metric_mapping_info->elts;
            for (j = i+1; j < metric_mapping_info->nelts; j++) {
                if (smi[j].mod_name == mi[i].mod_name) {
                    smi[j].mod_name = NULL;
                }
            }
        }
    }

    PERL_SYS_TERM();

    return APR_SUCCESS;
}

static g_val_t perl_metric_handler(int metric_index)
{
    g_val_t val;
    Ganglia_25metric *gmi = (Ganglia_25metric *) metric_info->elts;
    mapped_info_t *mi = (mapped_info_t*) metric_mapping_info->elts;
    int size;

    memset(&val, 0, sizeof(val));
    if (!mi[metric_index].pcb) {
        /* No call back provided for this metric */
        return val;
    }

    if (!mi[metric_index].perl) {
    	err_msg("No perl interpreter found.");
    	return val;
    }

    PERL_SET_CONTEXT(mi[metric_index].perl);

    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    PUTBACK;

    /* Call the metric handler call back for this metric */
    size = call_pv(mi[metric_index].pcb, G_NOARGS);
    SPAGAIN;

    if (SvTRUE(ERRSV)) {
        err_msg("[PERL] Can't call the metric handler function for [%s] in the perl module [%s].\n",
                gmi[metric_index].name, mi[metric_index].mod_name);
        /* return what? */
        return val;
    }
    else {
        /*SV* sref = POPs;
        if (!SvROK(sref)) {
            err_msg("[PERL] No values returned from metric handler function for [%s] in the perl module [%s].\n",
                    gmi[metric_index].name, mi[metric_index]. modname);
            return val;
        }*/
        if (size) {
            switch (gmi[metric_index].type) {
                case GANGLIA_VALUE_STRING:
                {
                    snprintf(val.str, sizeof(val.str), "%s", POPpx);
                    break;
                }
                case GANGLIA_VALUE_UNSIGNED_INT:
                {   
                    val.uint32 = POPl; 
                    break;
                }
                case GANGLIA_VALUE_INT:
                {
                    val.int32 = POPi;
                    break;
                }
                case GANGLIA_VALUE_FLOAT:
                {
                    /* XXX FIXEME */
                    val.f = POPn;
                    break;
                }
                case GANGLIA_VALUE_DOUBLE:
                {   
                    /* XXX FIXEME */
                    val.d = POPn;
                    break;
                }
                default:
                {
                    memset(&val, 0, sizeof(val));
                    break;
                }
            }
        }
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return val;
}

mmodule perl_module =
{
    STD_MMODULE_STUFF,
    perl_metric_init,
    NULL,
    NULL, /* defined dynamically */
    perl_metric_handler,
};
