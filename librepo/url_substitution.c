/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <glib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "url_substitution.h"
#include "util.h"

static LrVar *
lr_var_new(const char *var, const char *val)
{
    LrVar *var_val = lr_malloc0(sizeof(LrVar));
    var_val->var = g_strdup(var);
    var_val->val = g_strdup(val);
    return var_val;
}

static void
lr_var_free(LrVar *var)
{
    lr_free(var->var);
    lr_free(var->val);
    lr_free(var);
}

LrUrlVars *
lr_urlvars_set(LrUrlVars *list, const char *var, const char *value)
{
    LrUrlVars *ret = list;

    assert(var);

    if (!value) {
        // Remove var from the list
        for (LrUrlVars *elem = list; elem; elem = g_slist_next(elem)) {
            LrVar *var_val = elem->data;
            if (!strcmp(var, var_val->var)) {
                ret = g_slist_remove(list, var_val);
                lr_var_free(var_val);
                return ret;
            }
        }
    } else {
        // Replace var
        for (LrUrlVars *elem = list; elem; elem = g_slist_next(elem)) {
            LrVar *var_val = elem->data;
            if (!strcmp(var, var_val->var)) {
                lr_free(var_val->val);
                var_val->val = g_strdup(value);
                return ret;
            }
        }

        // Add var
        LrVar *var_val = lr_var_new(var, value);
        ret = g_slist_prepend(list, var_val);
    }

    return ret;
}

void
lr_urlvars_free(LrUrlVars *list)
{
    if (!list)
        return;
    for (LrUrlVars *elem = list; elem; elem = g_slist_next(elem))
        lr_var_free(elem->data);
    g_slist_free(list);
}

char *
lr_url_substitute(const char *url, LrUrlVars *list)
{
    const char *cur = url;
    const char *p = url;
    char *tmp_res, *res;

    if (!url)
        return NULL;

    if (!list)
        return g_strdup(url);

    res = g_strdup("");

    while (*cur != '\0') {
        if (*cur == '$') {
            if (cur-p) {
                char *tmp = g_strndup(p, cur-p);
                tmp_res = g_strconcat(res, tmp, NULL);
                lr_free(tmp);
                lr_free(res);
                res = tmp_res;
                p = cur;
            }

            // Try to substitute the variable
            for (LrUrlVars *elem = list; elem; elem = g_slist_next(elem)) {
                LrVar *var_val = elem->data;
                size_t len = strlen(var_val->var);
                if (!strncmp(var_val->var, (cur+1), len)) {
                    cur = cur + len;
                    p = cur + 1;
                    tmp_res = g_strconcat(res, var_val->val, NULL);
                    lr_free(res);
                    res = tmp_res;
                    break;
                }
            }
        }

        ++cur;
    }

    if (*p != '\0') {
        tmp_res = g_strconcat(res, p, NULL);
        lr_free(res);
        res = tmp_res;
    }

    return res;
}
