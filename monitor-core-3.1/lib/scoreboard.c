#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ganglia_priv.h"
#include "gm_msg.h"
#include "gm_scoreboard.h"
#include <apr_hash.h>
#include <apr_strings.h>

/* Author: Brad Nicholes (bnicholes novell.com) */

#ifdef GSTATUS

#define GSB_ERROR_MSG "ERROR the scoreboard has not been initialized.\n"

struct gsb_element {
  ganglia_scoreboard_types type;
  char *name;
  int val;
};
typedef struct gsb_element gsb_element;

static apr_hash_t *gsb_scoreboard = NULL;
static apr_pool_t *gsb_pool = NULL;

static gsb_element* get_scoreboard_element(char *name)
{
    gsb_element *element = NULL;

    if (gsb_scoreboard) {
        element = (gsb_element *)apr_hash_get(gsb_scoreboard, name, 
                                                           APR_HASH_KEY_STRING);
        if (!element) {
            debug_msg("%s does not exist in the scoreboard.\n", name);
        }
    }
    return element;
}

void ganglia_scoreboard_init(apr_pool_t *pool)
{
    if (!gsb_scoreboard) {
        if (apr_pool_create(&gsb_pool, pool) == APR_SUCCESS) {
            gsb_scoreboard = apr_hash_make(gsb_pool);
        }
        else {
            err_msg("ERROR creating the gmond scoreboard.\n");
        }
    }
}

void* ganglia_scoreboard_iterator()
{
    return (void*)apr_hash_first(gsb_pool, gsb_scoreboard);
}

char* ganglia_scoreboard_next(void **intr)
{
    apr_hash_index_t **hi = (apr_hash_index_t **)intr;
    char *name = NULL;
    gsb_element *element = NULL;

    if (*hi) {
        apr_hash_this(*hi, NULL, NULL, (void**)&element);
        name = element->name;
        *intr = (void*)apr_hash_next(*hi);
    }
    return name;
}

void ganglia_scoreboard_add(char *name, ganglia_scoreboard_types type)
{
    if (gsb_scoreboard) {
        gsb_element *element = apr_pcalloc(gsb_pool, sizeof(gsb_element));
        if (element) {
            element->type = type;
            element->name = apr_pstrdup(gsb_pool, name);
            element->val = 0;
            apr_hash_set(gsb_scoreboard, name, APR_HASH_KEY_STRING, element);
        }
        else {
            err_msg("ERROR scoreboard could not allocate memory.\n");
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
}

int ganglia_scoreboard_get(char *name)
{
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element) {
            int retval = element->val;
            if (element->type == GSB_READ_RESET) {
                element->val = 0;
            }
            return retval;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
    return 0;
}

void ganglia_scoreboard_set(char *name, int val)
{
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element) {
            element->val = val;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
}

void ganglia_scoreboard_reset(char *name)
{
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element) {
            element->val = 0;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
}

int ganglia_scoreboard_inc(char *name)
{
    int retval = 0;
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element && (element->type != GSB_STATE)) {
            element->val++;
            retval = element->val;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
    return retval;
}

void ganglia_scoreboard_dec(char *name)
{
    int retval = 0;
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element && (element->type != GSB_STATE)) {
            element->val--;
            if (element->val < 0)
                element->val = 0;
            retval = element->val;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
}

ganglia_scoreboard_types ganglia_scoreboard_type(char *name)
{
    if (gsb_scoreboard) {
        gsb_element *element = get_scoreboard_element(name);
        if (element) {
            return element->type;
        }
    }
    else {
        err_msg(GSB_ERROR_MSG);
    }
    return GSB_UNKNOWN;
}

#endif



