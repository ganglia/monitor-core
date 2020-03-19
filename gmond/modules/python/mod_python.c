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
*         Jon Carey (jcarey novell.com)
******************************************************************************/

#include <Python.h>
#include <gm_metric.h>
#include <gm_msg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_lib.h>

#include <dirent.h>
#include <sys/stat.h>

/*
 * Backward compatibility for 2.1 to 2.4
 */

#if PY_MAJOR_VERSION == 2
#if PY_MINOR_VERSION < 5
#define Py_ssize_t int
#endif
#if PY_MINOR_VERSION < 3
#define PyInt_AsUnsignedLongMask PyInt_AsLong
#endif
#endif
#if PY_MAJOR_VERSION >= 3
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#define PyInt_AsLong             PyLong_AsLong
#define PyString_AsString        PyThreeString_AsString
#define PyString_FromString      PyThreeString_FromString
#define PyInt_Check              PyLong_Check
#define PyString_Check           PyThreeStringCheck
#endif

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule python_module;

typedef struct
{
    PyObject* pmod;     /* The python metric module object */
    PyObject* pcb;      /* The metric call back function */
    char *mod_name;     /* The name */
}
mapped_info_t;          

typedef struct
{
    char mname[251];  // 255 (a fairly common filename max) - 4 (".rrd")
    int tmax;
    char vtype[32];
    char units[64];
    char slope[32];
    char format[64];
    char desc[512];
    char groups[512];
    apr_table_t *extra_data;
    PyObject* pcb;
}
py_metric_init_t;


static apr_pool_t *pool;
static PyThreadState* gtstate = NULL;

static apr_array_header_t *metric_info = NULL;
static apr_array_header_t *metric_mapping_info = NULL;
static apr_status_t pyth_metric_cleanup ( void *data);

char modname_bfr[PATH_MAX];
static char* is_python_module(const char* fname)
{
    char* p = strrchr(fname, '.');
    if (!p) {
        return NULL;
    }

    if (strcmp(p, ".py")) {
        return NULL;
    }

    strncpy(modname_bfr, fname, p-fname);
    modname_bfr[p-fname] = 0;
    return modname_bfr;
}

static int PyThreeStringCheck(PyObject* dv)
{
    if (PyUnicode_Check(dv)) {
      return 1;
    } else if (PyBytes_Check(dv)) {
      return 1;
    } else {
      return 0;
    }
}

static char* PyThreeString_AsString(PyObject* dv)
{
    if (PyUnicode_Check(dv)) {
        PyObject * temp_bytes = PyUnicode_AsEncodedString(dv, "UTF-8", "strict"); // Owned reference
        if (temp_bytes != NULL) {
            char* result = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
            result = strdup(result);
            Py_DECREF(temp_bytes);
            return result;
        } else {
            return NULL;
        }
    } else if (PyBytes_Check(dv)) {
        char* result = PyBytes_AS_STRING(dv); // Borrowed pointer
        result = strdup(result);
        return result;
    } else {
      return NULL;
    }
}

static PyObject* PyThreeString_FromString(const char* v)
{
    PyObject* dv = PyUnicode_FromString(v);
    return dv;
}




int
get_python_string_value(PyObject* dv, char* bfr, int len)
{
    int cc = 1;
    if (PyLong_Check(dv)) {
        long v = PyLong_AsLong(dv);
        snprintf(bfr, len, "%ld", v);
    }
    else if (PyInt_Check(dv)) {
        long v = PyInt_AsLong(dv);
        snprintf(bfr, len, "%ld", v);
    }
    else if (PyString_Check(dv)) {
        char* v = PyString_AsString(dv);
        snprintf(bfr, len, "%s", v);
    }
    else if (PyFloat_Check(dv)) {
        double v = PyFloat_AsDouble(dv);
        snprintf(bfr, len, "%f", v);
    }
    else {
        /* Don't know how to convert */
        cc = -1;
    }
    return cc;
}


/* Get a string value from a python dictionary
 * Return:
 *  0 - Item not in dictionary
 *  -1 - Invalid data type for key in dictionary
 *  1 - Success
 */
int
get_pydict_string_value(PyObject* pdict, char* key, char* bfr, int len)
{
    PyObject* dv;
    int cc = 1;
    if (!PyMapping_HasKeyString(pdict, key))
        return 0;   /* Key not in dictionary */
    dv = PyMapping_GetItemString(pdict, key);
    if (!dv)
        return 0;   /* shouldn't happen */
    cc = get_python_string_value(dv, bfr, len);
    Py_DECREF(dv);
    return cc;
}


int
get_python_uint_value(PyObject* dv, unsigned int* pint)
{
    int cc = 1;
    if (PyInt_Check(dv) || PyLong_Check(dv)) {
        unsigned long v = PyInt_AsUnsignedLongMask(dv);
        *pint = (unsigned int)v;
    }
    else if (PyString_Check(dv)) {
        /* Convert from string to int */
        unsigned long tid;
        char *endptr;
        char* p = PyString_AsString(dv);
        tid = strtoul(p, &endptr, 10);
        if(endptr == p || *endptr)
            cc = -1;    /* Invalid numeric format */
        else
            *pint = (unsigned int)tid;
    }
    else {
        cc = -1;    /* Don't know how to convert this */
    }
    return cc;
}

int
get_python_int_value(PyObject* dv, int* pint)
{
    int cc = 1;
    if (PyLong_Check(dv)) {
        long v = PyLong_AsLong(dv);
        *pint = (int)v;
    }
    else if (PyInt_Check(dv)) {
        long v = PyInt_AsLong(dv);
        *pint = (int)v;
    }
    else if (PyString_Check(dv)) {
        /* Convert from string to int */
        long tid;
        char *endptr;
        char* p = PyString_AsString(dv);
        tid = strtol(p, &endptr, 10);
        if(endptr == p || *endptr)
            cc = -1;    /* Invalid numeric format */
        else
            *pint = (int)tid;
    }
    else {
        cc = -1;    /* Don't know how to convert this */
    }
    return cc;
}

/* Return:
 *  0 - Item not in dictionary
 *  -1 - Invalid data type for key in dictionary
 *  1 - Success
 */
int
get_pydict_int_value(PyObject* pdict, char* key, int* pint)
{
    PyObject* dv;
    int cc = 1;
    if (!PyMapping_HasKeyString(pdict, key))
        return 0;
    dv = PyMapping_GetItemString(pdict, key);
    if (!dv)
        return 0;   /* shouldn't happen */
    cc = get_python_int_value(dv, pint);
    Py_DECREF(dv);
    return cc;
}

int
get_python_float_value(PyObject* dv, double* pnum)
{
    int cc = 1;
    if (PyFloat_Check(dv)) {
        *pnum = PyFloat_AsDouble(dv);
    }
    else if (PyLong_Check(dv)) {
        long v = PyLong_AsLong(dv);
        *pnum = (double)v;
    }
    else if (PyInt_Check(dv)) {
        long v = PyInt_AsLong(dv);
        *pnum = (double)v;
    }
    else if (PyString_Check(dv)) {
        /* Convert from string to int */
        double tid;
        char *endptr;
        char* p = PyString_AsString(dv);
        tid = strtod(p, &endptr);
        if(endptr == p || *endptr)
            cc = -1;    /* Invalid format for double */
        else
            *pnum = tid;
    }
    else {
        cc = -1;    /* Don't know how to convert this */
    }
    return cc;
}

/* Return:
 *  0 - Item not in dictionary
 *  -1 - Invalid data type for key in dictionary
 *  1 - Success
 */
int
get_pydict_float_value(PyObject* pdict, char* key, double* pnum)
{
    int cc = 1;
    PyObject* dv;
    if (!PyMapping_HasKeyString(pdict, key))
        return 0;
    dv = PyMapping_GetItemString(pdict, key);
    if (!dv)
        return 0;   /* shouldn't happen */
    cc = get_python_float_value(dv, pnum);
    Py_DECREF(dv);
    return cc;
}

/* Return:
 *  0 - Item not in dictionary
 *  -1 - Invalid data type for key in dictionary
 *  1 - Success
 */
int
get_pydict_callable_value(PyObject* pdict, char* key, PyObject** pobj)
{
    PyObject* dv;
    *pobj = NULL;
    if (!PyMapping_HasKeyString(pdict, key))
        return 0;
    dv = PyMapping_GetItemString(pdict, key);
    if (!dv)
        return 0;   /* shouldn't happen */
    if (!PyCallable_Check(dv))
    {
        Py_DECREF(dv);
        return -1;
    }
    *pobj = dv;
    return 1;
}

static void fill_metric_info(PyObject* pdict, py_metric_init_t* minfo, char *modname, apr_pool_t *pool)
{
    char *metric_name = "";
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int ret;
    char strkey[1024], strvalue[1024];

    memset(minfo, 0, sizeof(*minfo));

    /* create the apr table here */
    minfo->extra_data = apr_table_make(pool, 2);

    while (PyDict_Next(pdict, &pos, &key, &value)) {
        ret = get_python_string_value(key, strkey, sizeof(strkey));
        if (ret < 0) 
            continue;

        if (!strcasecmp(strkey, "name")) {
            if (get_python_string_value(value, minfo->mname, sizeof(minfo->mname)) < 1) {
                err_msg("[PYTHON] No metric name given in module [%s].\n", modname);
            }
            else
                metric_name = minfo->mname;
            continue;
        }

        if (!strcasecmp(strkey, "call_back")) {
            if (get_pydict_callable_value(pdict, "call_back", &minfo->pcb) < 1) {
                err_msg("[PYTHON] No python call back given for metric [%s] in module [%s]. Will not call\n", 
                        metric_name, modname);
            }
            continue;
        }
        if (!strcasecmp(strkey, "time_max")) {
            if (get_python_int_value(value, &(minfo->tmax)) < 1) {
                minfo->tmax = 60;
                err_msg("[PYTHON] No time max given for metric [%s] in module [%s]. Using %d.\n", 
                        metric_name, modname, minfo->tmax);
            }
            continue;
        }
        if (!strcasecmp(strkey, "value_type")) {
            if (get_python_string_value(value, minfo->vtype, sizeof(minfo->vtype)) < 1) {
                strcpy (minfo->vtype, "uint");
                err_msg("[PYTHON] No value type given for metric [%s] in module [%s]. Using %s.\n", 
                        metric_name, modname, minfo->vtype);
            }
            continue;
        }
        if (!strcasecmp(strkey, "units")) {
            if (get_python_string_value(value, minfo->units, sizeof(minfo->units)) < 1) {
                strcpy (minfo->units, "unknown");
                err_msg("[PYTHON] No metric units given for metric [%s] in module [%s]. Using %s.\n", 
                        metric_name, modname, minfo->units);
            }
            continue;
        }
        if (!strcasecmp(strkey, "slope")) {
            if (get_python_string_value(value, minfo->slope, sizeof(minfo->slope)) < 1) {
                strcpy (minfo->slope, "both");
                err_msg("[PYTHON] No slope given for metric [%s] in module [%s]. Using %s.\n", 
                        metric_name, modname, minfo->slope);
            }
            continue;
        }
        if (!strcasecmp(strkey, "format")) {
            if (get_python_string_value(value, minfo->format, sizeof(minfo->format)) < 1) {
                strcpy (minfo->format, "%u");
                err_msg("[PYTHON] No format given for metric [%s] in module [%s]. Using %s.\n", 
                        metric_name, modname, minfo->format);
            }
            continue;
        }
        if (!strcasecmp(strkey, "description")) {
            if (get_python_string_value(value, minfo->desc, sizeof(minfo->desc)) < 1) {
                strcpy (minfo->desc, "unknown metric");
                err_msg("[PYTHON] No description given for metric [%s] in module [%s]. Using %s.\n", 
                        metric_name, modname, minfo->desc);
            }
            continue;
        }
        if (!strcasecmp(strkey, "groups")) {
            if (get_python_string_value(value, minfo->groups, sizeof(minfo->groups)) < 1) {
                strcpy (minfo->groups, "");
            }
            continue;
        }

        if (get_python_string_value(value, strvalue, sizeof(strvalue)) < 1) {
            err_msg("[PYTHON] Extra data key [%s] could not be processed.\n", strkey); 
        }
        else {
            apr_table_add(minfo->extra_data, strkey, strvalue);
        }
    }
}

static void fill_gmi(Ganglia_25metric* gmi, py_metric_init_t* minfo)
{
    char *s, *lasts;
    int i;
    const apr_array_header_t *arr = apr_table_elts(minfo->extra_data);
    const apr_table_entry_t *elts = (const apr_table_entry_t *)arr->elts;

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

    modules_cfg = cfg_getsec(python_module.config_file, "modules");
    for (j = 0; j < cfg_size(modules_cfg, "module"); j++) {
        char *modName, *modLanguage;
        int modEnabled;

        cfg_t *pymodule = cfg_getnsec(modules_cfg, "module", j);

        /* Check the module language to make sure that
           the language designation is python.
        */
        modLanguage = cfg_getstr(pymodule, "language");
        if (!modLanguage || strcasecmp(modLanguage, "python")) 
            continue;

        modName = cfg_getstr(pymodule, "name");
        if (strcasecmp(modname, modName)) {
            continue;
        }

        /* Check to make sure that the module is enabled.
        */
        modEnabled = cfg_getbool(pymodule, "enabled");
        if (!modEnabled) 
            continue;

        return pymodule;
    }
    return NULL; 
}

static PyObject* build_params_dict(cfg_t *pymodule)
{
    int k;
    PyObject *params_dict = PyDict_New();

    if (pymodule && params_dict) {
        for (k = 0; k < cfg_size(pymodule, "param"); k++) {
            cfg_t *param;
            char *name, *value;
            PyObject *pyvalue;
    
            param = cfg_getnsec(pymodule, "param", k);
            name = apr_pstrdup(pool, param->title);
            value = apr_pstrdup(pool, cfg_getstr(param, "value"));
            pyvalue = PyString_FromString(value);
            if (name && pyvalue) {
                PyDict_SetItemString(params_dict, name, pyvalue);
                Py_DECREF(pyvalue);
            }
        }
    }
    return params_dict;
}

static PyObject*
ganglia_get_debug_msg_level(PyObject *self, PyObject *args)
{
    return Py_BuildValue("i", get_debug_msg_level());
}

static PyMethodDef GangliaMethods[] = {
    {"get_debug_msg_level", ganglia_get_debug_msg_level, METH_NOARGS,
     "Return the debug level used by ganglia."},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef gangliaModPy =
{
    PyModuleDef_HEAD_INIT,
    "ganglia", /* name of module */
    NULL,          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    GangliaMethods
};
#endif

static int pyth_metric_init (apr_pool_t *p)
{
    DIR *dp;
    struct dirent *entry;
    int i;
    char* modname;
    PyObject *pmod, *pinitfunc, *pobj, *pparamdict;
    py_metric_init_t minfo;
    Ganglia_25metric *gmi;
    mapped_info_t *mi;
    const char* path = python_module.module_params;
    //const char* path = "/root/cpythontest/conf/modpython.conf";
    cfg_t *module_cfg;

    /* Allocate a pool that will be used by this module */
    apr_pool_create(&pool, p);

    metric_info = apr_array_make(pool, 10, sizeof(Ganglia_25metric));
    metric_mapping_info = apr_array_make(pool, 10, sizeof(mapped_info_t));

    /* Verify path exists and can be read */

    if (!path) {
        err_msg("[PYTHON] Missing python module path.\n");
        return -1;
    }

    if (access(path, F_OK))
    {
        /* 'path' does not exist */
        err_msg("[PYTHON] Can't open the python module path %s.\n", path);
        return -1;
    }

    if (access(path, R_OK))
    {
        /* Don't have read access to 'path' */
        err_msg("[PYTHON] Can't read from the python module path %s.\n", path);
        return -1;
    }

    /* Init Python environment */

    /* Set up the python path to be able to load module from our module path */
    Py_Initialize();
    #if PY_MAJOR_VERSION >= 3
      PyModule_Create(&gangliaModPy);
    #else
      Py_InitModule("ganglia", GangliaMethods);
    #endif

    PyObject *sys_path = PySys_GetObject("path");
    PyObject *addpath = PyString_FromString(path);
    PyList_Append(sys_path, addpath);

    PyEval_InitThreads();
    gtstate = PyEval_SaveThread();

    /* Initialize each python module */
    if ((dp = opendir(path)) == NULL) {
        /* Error: Cannot open the directory - Shouldn't happen */
        /* Log? */
        err_msg("[PYTHON] Can't open the python module path %s.\n", path);
        return -1;
    }

    i = 0;

    while ((entry = readdir(dp)) != NULL) {
        modname = is_python_module(entry->d_name);

        if (modname == NULL)
            continue;

        /* Find the specified module configuration in gmond.conf 
           If this return NULL then either the module config
           doesn't exist or the module is disabled. */
        module_cfg = find_module_config(modname);
        if (!module_cfg)
            continue;

        PyEval_RestoreThread(gtstate);

        pmod = PyImport_ImportModule(modname);
        if (!pmod) {
            /* Failed to import module. Log? */
            err_msg("[PYTHON] Can't import the metric module [%s].\n", modname);
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            gtstate = PyEval_SaveThread();
            continue;
        }

        pinitfunc = PyObject_GetAttrString(pmod, "metric_init");
        if (!pinitfunc || !PyCallable_Check(pinitfunc)) {
            /* No metric_init function. */
            err_msg("[PYTHON] Can't find the metric_init function in the python module [%s].\n", modname);
            Py_DECREF(pmod);
            gtstate = PyEval_SaveThread();
            continue;
        }

        /* Build a parameter dictionary to pass to the module */
        pparamdict = build_params_dict(module_cfg);
        if (!pparamdict || !PyDict_Check(pparamdict)) {
            /* No metric_init function. */
            err_msg("[PYTHON] Can't build the parameters dictionary for [%s].\n", modname);
            Py_DECREF(pmod);
            gtstate = PyEval_SaveThread();
            continue;
        }

        /* Now call the metric_init method of the python module */
        pobj = PyObject_CallFunction(pinitfunc, "(N)", pparamdict);

        if (!pobj) {
            /* failed calling metric_init */
            err_msg("[PYTHON] Can't call the metric_init function in the python module [%s].\n", modname);
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            Py_DECREF(pinitfunc);
            Py_DECREF(pmod);
            gtstate = PyEval_SaveThread();
            continue;
        }

        if (PyList_Check(pobj)) {
            int j;
            int size = PyList_Size(pobj);
            for (j = 0; j < size; j++) {
                PyObject* plobj = PyList_GetItem(pobj, j);
                if (PyMapping_Check(plobj)) {
                    fill_metric_info(plobj, &minfo, modname, pool);
                    gmi = (Ganglia_25metric*)apr_array_push(metric_info);
                    fill_gmi(gmi, &minfo);
                    mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
                    mi->pmod = pmod;
                    mi->mod_name = apr_pstrdup(pool, modname);
                    mi->pcb = minfo.pcb;
                }
            }
        }
        else if (PyMapping_Check(pobj)) {
            fill_metric_info(pobj, &minfo, modname, pool);
            gmi = (Ganglia_25metric*)apr_array_push(metric_info);
            fill_gmi(gmi, &minfo);
            mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
            mi->pmod = pmod;
            mi->mod_name = apr_pstrdup(pool, modname);
            mi->pcb = minfo.pcb;
        }
        Py_DECREF(pobj);
        Py_DECREF(pinitfunc);
        gtstate = PyEval_SaveThread();
    }
    closedir(dp);

    apr_pool_cleanup_register(pool, NULL,
                              pyth_metric_cleanup,
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

    python_module.metrics_info = (Ganglia_25metric *)metric_info->elts;
    return 0;
}

static apr_status_t pyth_metric_cleanup ( void *data)
{
    PyObject *pcleanup, *pobj;
    mapped_info_t *mi, *smi;
    int i, j;

    mi = (mapped_info_t*) metric_mapping_info->elts;
    for (i = 0; i < metric_mapping_info->nelts; i++) {
        if (mi[i].pmod) {
            PyEval_RestoreThread(gtstate);
            pcleanup = PyObject_GetAttrString(mi[i].pmod, "metric_cleanup");
            if (pcleanup && PyCallable_Check(pcleanup)) {
                pobj = PyObject_CallFunction(pcleanup, NULL);
                Py_XDECREF(pobj);
                if (PyErr_Occurred()) {
                    PyErr_Print();
                }
            }
            Py_XDECREF(pcleanup);
            Py_DECREF(mi[i].pmod);
            Py_XDECREF(mi[i].pcb);
            gtstate = PyEval_SaveThread();

            /* Set all modules that fall after this once with the same
             * module pointer to NULL so metric_cleanup only gets called
             * once on the module.
             */
            smi = (mapped_info_t*) metric_mapping_info->elts;
            for (j = i+1; j < metric_mapping_info->nelts; j++) {
                if (smi[j].pmod == mi[i].pmod) {
                    smi[j].pmod = NULL;
                }
            }
        }
    }

    PyEval_RestoreThread(gtstate);
    Py_Finalize();
    return APR_SUCCESS;
}

static g_val_t pyth_metric_handler( int metric_index )
{
    g_val_t val;
    PyObject *pobj;
    Ganglia_25metric *gmi = (Ganglia_25metric *) metric_info->elts;
    mapped_info_t *mi = (mapped_info_t*) metric_mapping_info->elts;

    memset(&val, 0, sizeof(val));
    if (!mi[metric_index].pcb) {
        /* No call back provided for this metric */
        return val;
    }

    PyEval_RestoreThread(gtstate);

    /* Call the metric handler call back for this metric */
    pobj = PyObject_CallFunction(mi[metric_index].pcb, "s", gmi[metric_index].name);
    if (!pobj) {
        err_msg("[PYTHON] Can't call the metric handler function for [%s] in the python module [%s].\n", 
                gmi[metric_index].name, mi[metric_index].mod_name);
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        gtstate = PyEval_SaveThread();
        /* return what? */
        return val;
    }

    switch (gmi[metric_index].type) {
        case GANGLIA_VALUE_STRING:
        {
            get_python_string_value(pobj, val.str, sizeof(val.str));
            break;
        }
        case GANGLIA_VALUE_UNSIGNED_INT:
        {
            unsigned int v = 0;
            get_python_uint_value(pobj, &v);
            val.uint32 = v;
            break;
        }
        case GANGLIA_VALUE_INT:
        {
            int v = 0;
            get_python_int_value(pobj, &v);
            val.int32 = v;
            break;
        }
        case GANGLIA_VALUE_FLOAT:
        {
            double v = 0.0;
            get_python_float_value(pobj, &v);
            val.f = v;
            break;
        }
        case GANGLIA_VALUE_DOUBLE:
        {
            double v = 0.0;
            get_python_float_value(pobj, &v);
            val.d = v;
            break;
        }
        default:
        {
            memset(&val, 0, sizeof(val));
            break;
        }
    }
    Py_DECREF(pobj);
    gtstate = PyEval_SaveThread();
    return val;
}

mmodule python_module =
{
    STD_MMODULE_STUFF,
    pyth_metric_init,
    NULL,
    NULL, /* defined dynamically */
    pyth_metric_handler,
};