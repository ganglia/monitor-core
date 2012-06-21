/*******************************************************************************
*
* This code is part of a php module for ganglia.
*
* Author : Nicolas Brousse (nicolas brousse.info)
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
******************************************************************************/

#include <gm_metric.h>
#include <gm_msg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "file.h"

#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_lib.h>

#include <dirent.h>
#include <sys/stat.h>

#include <sapi/embed/php_embed.h>

#ifdef ZTS
	void ***tsrm_ls;
#endif

#define php_verbose_debug(debug_level, ...) { \
	if (get_debug_msg_level() > debug_level) { \
		debug_msg(__VA_ARGS__); \
	} \
}

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule php_module;

typedef struct
{
    zval *phpmod;     /* The php metric module object */
    char *callback;   /* The metric call back function */
    char *mod_name;   /* The name */
    char *script; 	  /* PHP script filename to run */
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
    char *callback;
}
php_metric_init_t;

static apr_pool_t *pool;

static apr_array_header_t *metric_info = NULL;
static apr_array_header_t *metric_mapping_info = NULL;
static apr_status_t php_metric_cleanup ( void *data);

char modname_bfr[PATH_MAX];
static char* is_php_module(const char* fname)
{
	php_verbose_debug(3, "is_php_module");
    char* p = strrchr(fname, '.');
    if (!p) {
        return NULL;
    }

    if (strcmp(p, ".php")) {
        return NULL;
    }

    strncpy(modname_bfr, fname, p-fname);
    modname_bfr[p-fname] = 0;
    return modname_bfr;
}

static void fill_metric_info(zval* dict, php_metric_init_t* minfo, char *modname, apr_pool_t *pool)
{
    char *metric_name = "";
    char *key;
    uint keylen;
    ulong idx;
    HashPosition pos;
    zval **current;

	php_verbose_debug(3, "fill_metric_info");

    memset(minfo, 0, sizeof(*minfo));

    /* create the apr table here */
    minfo->extra_data = apr_table_make(pool, 2);

    zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(dict), &pos);
    for(;; zend_hash_move_forward_ex(Z_ARRVAL_P(dict), &pos)) {

        if (zend_hash_get_current_key_ex(Z_ARRVAL_P(dict), &key, &keylen, &idx, 0, &pos) == HASH_KEY_NON_EXISTANT)
            break;

    	if (zend_hash_get_current_data_ex(Z_ARRVAL_P(dict), (void**) &current, &pos) == FAILURE) {
            err_msg("[PHP] Can't get data for key [%s] in php module [%s].\n", key, modname);
            continue;
    	}

        if (!strcasecmp(key, "name")) {
        	if (!strncpy(minfo->mname, Z_STRVAL_PP(current), sizeof(minfo->mname))) {
                err_msg("[PHP] No metric name given in php module [%s].\n", modname);
            }
            else
                metric_name = minfo->mname;
            continue;
        }

        if (!strcasecmp(key, "call_back")) {
        	if (!(minfo->callback = pestrndup(Z_STRVAL_PP(current), Z_STRLEN_PP(current), 1))) {
                err_msg("[PHP] No php call back given for metric [%s] in module [%s]. Will not call\n",
                        metric_name, modname);
            }
            continue;
        }

        if (!strcasecmp(key, "time_max")) {
            if (!(minfo->tmax = (int) Z_LVAL_PP(current))) {
                minfo->tmax = 60;
                err_msg("[PHP] No time max given for metric [%s] in module [%s]. Using %d.\n",
                        metric_name, modname, minfo->tmax);
            }
            continue;
        }

        if (!strcasecmp(key, "value_type")) {
        	if (!strncpy(minfo->vtype, Z_STRVAL_PP(current), sizeof(minfo->vtype))) {
                strcpy (minfo->vtype, "uint");
                err_msg("[PHP] No value type given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->vtype);
            }
            continue;
        }

        if (!strcasecmp(key, "units")) {
        	if (!strncpy(minfo->units, Z_STRVAL_PP(current), sizeof(minfo->units))) {
                strcpy (minfo->units, "unknown");
                err_msg("[PHP] No metric units given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->units);
            }
            continue;
        }

        if (!strcasecmp(key, "slope")) {
        	if (!strncpy(minfo->slope, Z_STRVAL_PP(current), sizeof(minfo->slope))) {
                strcpy (minfo->slope, "both");
                err_msg("[PHP] No slope given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->slope);
            }
            continue;
        }

        if (!strcasecmp(key, "format")) {
        	if (!strncpy(minfo->format, Z_STRVAL_PP(current), sizeof(minfo->format))) {
                strcpy (minfo->format, "%u");
                err_msg("[PHP] No format given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->format);
            }
            continue;
        }

        if (!strcasecmp(key, "description")) {
        	if (!strncpy(minfo->desc, Z_STRVAL_PP(current), sizeof(minfo->desc))) {
                strcpy (minfo->desc, "unknown metric");
                err_msg("[PHP] No description given for metric [%s] in module [%s]. Using %s.\n",
                        metric_name, modname, minfo->desc);
            }
            continue;
        }

        if (!strcasecmp(key, "groups")) {
        	if (!strncpy(minfo->groups, Z_STRVAL_PP(current), sizeof(minfo->groups))) {
                strcpy (minfo->groups, "");
            }
            continue;
        }

        if (Z_TYPE_PP(current) == IS_LONG || Z_TYPE_PP(current) == IS_DOUBLE ||
        		Z_TYPE_PP(current) == IS_BOOL || Z_TYPE_PP(current) == IS_STRING) {
        	convert_to_string(*current);
            apr_table_add(minfo->extra_data, key, Z_STRVAL_P(*current));
        }
        else {
            err_msg("[PHP] Extra data key [%s] could not be processed.\n", key);
        }
    
    }

    php_verbose_debug(3, "name: %s", minfo->mname);
    php_verbose_debug(3, "callback: %s", minfo->callback);
    php_verbose_debug(3, "time_max: %d", minfo->tmax);
    php_verbose_debug(3, "value_type: %s", minfo->vtype);
    php_verbose_debug(3, "units: %s", minfo->units);
    php_verbose_debug(3, "slope: %s", minfo->slope);
    php_verbose_debug(3, "format: %s", minfo->format);
    php_verbose_debug(3, "description: %s", minfo->desc);
    php_verbose_debug(3, "groups: %s", minfo->groups);

}

static void fill_gmi(Ganglia_25metric* gmi, php_metric_init_t* minfo)
{
    char *s, *lasts;
    int i;
    const apr_array_header_t *arr = apr_table_elts(minfo->extra_data);
    const apr_table_entry_t *elts = (const apr_table_entry_t *)arr->elts;

    php_verbose_debug(3, "fill_gmi");

    /* gmi->key will be automatically assigned by gmond */
    gmi->name = apr_pstrdup (pool, minfo->mname);
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

static cfg_t* find_module_config(char *modname)
{
    cfg_t *modules_cfg;
    int j;

    php_verbose_debug(3, "find_module_config");

    modules_cfg = cfg_getsec(php_module.config_file, "modules");
    for (j = 0; j < cfg_size(modules_cfg, "module"); j++) {
        char *modName, *modLanguage;
        int modEnabled;

        cfg_t *phpmodule = cfg_getnsec(modules_cfg, "module", j);

        /* Check the module language to make sure that
           the language designation is PHP.
        */
        modLanguage = cfg_getstr(phpmodule, "language");
        if (!modLanguage || strcasecmp(modLanguage, "php"))
            continue;

        modName = cfg_getstr(phpmodule, "name");
        if (strcasecmp(modname, modName)) {
            continue;
        }

        /* Check to make sure that the module is enabled.
        */
        modEnabled = cfg_getbool(phpmodule, "enabled");
        if (!modEnabled)
            continue;

        return phpmodule;
    }
    return NULL;
}

static void build_params_dict(zval *params_dict, cfg_t *phpmodule)
{
    int k;

    /* init params_dict as a ZVAL ARRAY */
    array_init(params_dict);

    if (phpmodule) {
        for (k = 0; k < cfg_size(phpmodule, "param"); k++) {
            cfg_t *param;
            char *name, *value;

            param = cfg_getnsec(phpmodule, "param", k);
            name = apr_pstrdup(pool, param->title);
            value = apr_pstrdup(pool, cfg_getstr(param, "value"));
            if (name && value) {
            	add_assoc_string(params_dict, name, value, 1);
            }
        }
    }
}

static int php_metric_init (apr_pool_t *p)
{
	DIR *dp;
	struct dirent *entry;
	char* modname;
	php_metric_init_t minfo;
	Ganglia_25metric *gmi;
	mapped_info_t *mi;
	const apr_array_header_t *php_module_params = php_module.module_params_list;
	char* path = NULL;
	char* php_ini_path = NULL;
	cfg_t *module_cfg;
    char *key;
    uint keylen;
    ulong idx;
    HashPosition pos;
	zval retval, funcname, *params, **current, **zval_vector[1];
	zend_uint params_length;
	zend_file_handle script;
    char file[256];
    int i, php_initialized = 0;

	php_verbose_debug(3, "php_metric_init");
	php_verbose_debug(3, "php_module_params size : %d", php_module_params->nelts);

	for (i=0; i < php_module_params->nelts; ++i) {

		mmparam node = ((mmparam *)php_module_params->elts)[i];

		php_verbose_debug(3, "php_module_params: Key=%s, Value=%s", node.name, node.value);

		if (!strcasecmp(node.name, "php_modules_path")) {
			path = node.value;
			php_verbose_debug(2, "php_modules path: %s", path);
		} else if (!strcasecmp(node.name, "php_ini_path")) {
			php_ini_path = node.value;
		} else {
			php_verbose_debug(1, "Unknown PHP module param : %s", node.name);
		}

	}

	/* Allocate a pool that will be used by this module */
	apr_pool_create(&pool, p);

	metric_info = apr_array_make(pool, 10, sizeof(Ganglia_25metric));
	metric_mapping_info = apr_array_make(pool, 10, sizeof(mapped_info_t));

	/* Verify path exists and can be read */

	if (!path) {
		err_msg("[PHP] Missing php modules path.\n");
		return -1;
	}

    if (access(path, F_OK)) {
        /* 'path' does not exist */
        err_msg("[PHP] Can't open the PHP module path %s.\n", path);
        return -1;
    }

    if (access(path, R_OK)) {
        /* Don't have read access to 'path' */
        err_msg("[PHP] Can't read from the PHP module path %s.\n", path);
        return -1;
    }

    /* Initialize each perl module */
    if ((dp = opendir(path)) == NULL) {
        /* Error: Cannot open the directory - Shouldn't happen */
        /* Log? */
        err_msg("[PHP] Can't open the PHP module path %s.\n", path);
        return -1;
    }

	php_verbose_debug(3, "php_embed_init");
	if (php_ini_path) {
		if (access(php_ini_path, R_OK)) {
	        err_msg("[PHP] Can't read the php.ini : %s.\n", php_ini_path);
	        return -1;
		} else {
			php_verbose_debug(2, "Using php.ini : %s", php_ini_path);
			php_embed_module.php_ini_path_override = php_ini_path;
		}
	}
	if (php_embed_init(0, NULL PTSRMLS_CC) != SUCCESS) {
		err_msg("[PHP] Can't start the PHP engine.");
		return -1;
	}
    php_verbose_debug(2, ">>> started php sapi %s", sapi_module.name);

    while ((entry = readdir(dp)) != NULL) {

        modname = is_php_module(entry->d_name);

        if (modname == NULL)
            continue;

        /* Find the specified module configuration in gmond.conf
           If this return NULL then either the module config
           doesn't exist or the module is disabled. */
        module_cfg = find_module_config(modname);
        if (!module_cfg)
            continue;

        /* start php engine if not started yet */
        zend_try {
        	if (!php_initialized) {
        		php_initialized = 1;
        	} else {
        		php_request_startup(TSRMLS_C);
        	}
        } zend_end_try();

        strcpy(file, path);
        strcat(file, "/");
        strcat(file, modname);
        strcat(file, ".php");

        script.type = ZEND_HANDLE_FP;
        script.filename = pestrndup((char *)&file, sizeof(file), 1);
        script.opened_path = NULL;
        script.free_filename = 0;
        if (!(script.handle.fp = fopen(script.filename, "rb"))) {
        	err_msg("Unable to open %s\n", script.filename);
        	continue;
        }

        php_verbose_debug(2, ">>> execute php script %s", script.filename);
        php_execute_script(&script TSRMLS_CC);

        /* Build a parameter dictionary to pass to the module */
        MAKE_STD_ZVAL(params);
        build_params_dict(params, module_cfg);
        if (!params || Z_TYPE_P(params) != IS_ARRAY) {
            /* No metric_init function. */
            err_msg("[PHP] Can't build the parameters array for [%s].\n", modname);
            continue;
        }
        php_verbose_debug(3, "built the parameters dictionnary for the php module [%s]", modname);

        ZVAL_STRING(&funcname, "metric_init", 0);
        params_length = zend_hash_num_elements(Z_ARRVAL_P(params));
        php_verbose_debug(2, "found %d params for the php module [%s]", params_length, modname);

        /* Convert params to zval vector */
        zval_vector[0] = &params;

        /* Now call the metric_init method of the python module */
        if (call_user_function(EG(function_table), NULL, &funcname, &retval,
        		1, *zval_vector TSRMLS_CC) == FAILURE) {
        	/* failed calling metric_init */
            err_msg("[PHP] Can't call the metric_init function in the php module [%s]\n", modname);
            continue;
        }

        php_verbose_debug(3, "called the metric_init function for the php module [%s]\n", modname);

        if (Z_TYPE_P(&retval) == IS_ARRAY) {
			php_verbose_debug(2, "get %d descriptors for the php module [%s]",
					zend_hash_num_elements(Z_ARRVAL(retval)), modname);

        	zend_hash_internal_pointer_reset_ex(Z_ARRVAL(retval), &pos);
        	while (zend_hash_get_current_data_ex(Z_ARRVAL(retval), (void**)&current, &pos) == SUCCESS) {

				if (zend_hash_get_current_key_ex(Z_ARRVAL(retval), &key, &keylen, &idx, 0, &pos) == HASH_KEY_NON_EXISTANT)
					break;

            	if (Z_TYPE_PP(current) == IS_ARRAY) {
                    fill_metric_info(*current, &minfo, modname, pool);
            		php_verbose_debug(3, "metric info [%s] (callback : %s)", modname, minfo.callback);
                    gmi = (Ganglia_25metric*)apr_array_push(metric_info);
                    fill_gmi(gmi, &minfo);
                    mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
                    mi->phpmod = *current;
                    mi->script = script.filename;
                    mi->mod_name = apr_pstrdup(pool, modname);
                    mi->callback = minfo.callback;
            	}

    			zend_hash_move_forward_ex(Z_ARRVAL(retval), &pos);
            }
        }

        zval_dtor(&retval);
        zval_ptr_dtor(&params);

        zend_try {
        	php_request_shutdown(NULL);
        } zend_end_try();

    }
    closedir(dp);

    apr_pool_cleanup_register(pool, NULL,
                              php_metric_cleanup,
                              apr_pool_cleanup_null);

    /* Replace the empty static metric definition array with the
       dynamic array that we just created
    */
    gmi = apr_array_push(metric_info);
    memset (gmi, 0, sizeof(*gmi));
    mi = apr_array_push(metric_mapping_info);
    memset (mi, 0, sizeof(*mi));

    php_module.metrics_info = (Ganglia_25metric *)metric_info->elts;

	return 0;
}

static apr_status_t php_metric_cleanup ( void *data)
{
    mapped_info_t *mi, *smi;
    int i, j;

	php_verbose_debug(3, "php_metric_cleanup");

    mi = (mapped_info_t*) metric_mapping_info->elts;
    for (i = 0; i < metric_mapping_info->nelts; i++) {
        if (mi[i].phpmod) {
        	//efree(mi[i].callback);
        	//pefree(mi[i].script, 1);
        	zval_ptr_dtor(&mi[i].phpmod);

            /* Set all modules that fall after this once with the same
             * module pointer to NULL so metric_cleanup only gets called
             * once on the module.
             */
            smi = (mapped_info_t*) metric_mapping_info->elts;
            for (j = i+1; j < metric_mapping_info->nelts; j++) {
                if (smi[j].phpmod == mi[i].phpmod) {
                    smi[j].phpmod = NULL;
                }
            }
        }
    }

    php_module_shutdown(TSRMLS_C);
    sapi_shutdown();
#ifdef ZTS
    tsrm_shutdown();
#endif
    if (php_embed_module.ini_entries) {
        free(php_embed_module.ini_entries);
        php_embed_module.ini_entries = NULL;
    }

    return APR_SUCCESS;
}

static g_val_t php_metric_handler( int metric_index )
{
	zval retval, funcname, *tmp, **zval_vector[1];
	zend_file_handle script;
    g_val_t val;
    Ganglia_25metric *gmi = (Ganglia_25metric *) metric_info->elts;
    mapped_info_t *mi = (mapped_info_t*) metric_mapping_info->elts;

	php_request_startup(TSRMLS_C);

    php_verbose_debug(3, "php_metric_handler");

    memset(&val, 0, sizeof(val));
    if (!mi[metric_index].callback) {
        /* No call back provided for this metric */
        return val;
    }

    php_verbose_debug(3, ">>> metric index : %d, callback : %s", metric_index, (char *) mi[metric_index].callback);

    script.type = ZEND_HANDLE_FP;
    script.filename = mi[metric_index].script;
    script.opened_path = NULL;
    script.free_filename = 0;
    if (!(script.handle.fp = fopen(script.filename, "rb"))) {
    	err_msg("Unable to open %s\n", script.filename);
    	return val;
    }

    php_verbose_debug(2, ">>> execute php script %s", script.filename);

    php_execute_script(&script TSRMLS_CC);

    ZVAL_STRING(&funcname, mi[metric_index].callback, 0);
    MAKE_STD_ZVAL(tmp);
    ZVAL_STRING(tmp, gmi[metric_index].name, 1);
    zval_vector[0] = &tmp;

    /* Call the metric handler call back for this metric */
    if (call_user_function(EG(function_table), NULL, &funcname, &retval,
    		1, *zval_vector TSRMLS_CC) == FAILURE) {
    	/* failed calling metric_init */
        err_msg("[PHP]  Can't call the metric handler function [%s] for [%s] in the php module [%s].\n",
        		&funcname, gmi[metric_index].name, mi[metric_index].mod_name);
        return val;
    }
    php_verbose_debug(3, "Called the metric handler function [%s] for [%s] in the php module [%s].\n",
    		mi[metric_index].callback, gmi[metric_index].name, mi[metric_index].mod_name);

    switch (gmi[metric_index].type) {
        case GANGLIA_VALUE_STRING:
        {
        	convert_to_string(&retval);
        	strcpy(val.str, Z_STRVAL_P(&retval));
        	php_verbose_debug(3, "string: %s", val.str);
            break;
        }
        case GANGLIA_VALUE_UNSIGNED_INT:
        {
        	convert_to_long(&retval);
            val.uint32 = (unsigned int) Z_LVAL_P(&retval);
        	php_verbose_debug(3, "uint: %i", val.uint32);
            break;
        }
        case GANGLIA_VALUE_INT:
        {
        	convert_to_long(&retval);
            val.int32 = (int) Z_LVAL_P(&retval);
        	php_verbose_debug(3, "int: %i", val.int32);
            break;
        }
        case GANGLIA_VALUE_FLOAT:
        {
        	convert_to_double(&retval);
            val.f = Z_DVAL_P(&retval);
        	php_verbose_debug(3, "float: %d", val.f);
            break;
        }
        case GANGLIA_VALUE_DOUBLE:
        {
        	convert_to_double(&retval);
            val.d = Z_DVAL_P(&retval);
        	php_verbose_debug(3, "double: %d", val.d);
            break;
        }
        default:
        {
        	break;
        }
    }

    zval_ptr_dtor(&tmp);

    php_request_shutdown(NULL);

    return val;
}

mmodule php_module =
{
    STD_MMODULE_STUFF,
    php_metric_init,
    NULL,
    NULL, /* defined dynamically */
    php_metric_handler,
};
