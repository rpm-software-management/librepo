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
#include <ctype.h>
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

    if (!url)
        return NULL;

    if (!list)
        return g_strdup(url);

    char *res = g_strdup("");

    while (*cur != '\0') {
        if (*cur == '$') {
            // Adds unprocessed text before the variable to the "res".
            if (cur-p) {
                char *tmp = g_strndup(p, cur-p);
                char *tmp_res = g_strconcat(res, tmp, NULL);
                g_free(tmp);
                g_free(res);
                res = tmp_res;
                p = cur;
            }

            // Tries to substitute the variable and store result to the "res".
            gboolean bracket;
            if (*++cur == '{') {
                bracket = TRUE;
                ++cur;
            } else {
                bracket = FALSE;
            }
            const char *varname = cur;
            for (; isalnum(*cur) || (*cur == '_' && isalnum(*(cur + 1))); ++cur);
            if (cur != varname && (!bracket || *cur == '}')) {
                for (LrUrlVars *elem = list; elem; elem = g_slist_next(elem)) {
                    LrVar *var_val = elem->data;
                    size_t var_len = strlen(var_val->var);
                    if (var_len == cur - varname && strncmp(var_val->var, varname, var_len) == 0) {
                        if (bracket)
                            ++cur;
                        p = cur;
                        char *tmp_res = g_strconcat(res, var_val->val, NULL);
                        g_free(res);
                        res = tmp_res;
                        break;
                    }
                }
            }
        } else {
            ++cur;
        }
    }

    // Adds remaining text to the "res".
    if (*p != '\0') {
        char *tmp_res = g_strconcat(res, p, NULL);
        g_free(res);
        res = tmp_res;
    }

    return res;
}
