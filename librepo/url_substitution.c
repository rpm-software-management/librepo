/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <assert.h>
#include <string.h>
#include "setup.h"
#include "url_substitution.h"
#include "list.h"
#include "util.h"

static lr_Var *
lr_var_new(const char *var, const char *val)
{
    lr_Var *var_val = lr_malloc0(sizeof(lr_Var));
    var_val->var = lr_strdup(var);
    var_val->val = lr_strdup(val);
    return var_val;
}

static void
lr_var_free(lr_Var *var)
{
    lr_free(var->var);
    lr_free(var->val);
    lr_free(var);
}

lr_UrlVars *
lr_urlvars_set(lr_UrlVars *urlvars, const char *var, const char *value)
{
    lr_UrlVars *ret = urlvars;

    assert(var);

    if (!value) {
        // Remove var from the list
        for (lr_List *elem = urlvars; elem; elem = lr_list_next(elem)) {
            lr_Var *var_val = lr_list_data(elem);
            if (!strcmp(var, var_val->var)) {
                lr_var_free(var_val);
                ret = lr_list_remove(urlvars, var_val);
                return ret;
            }
        }
    } else {
        // Replace var
        for (lr_List *elem = urlvars; elem; elem = lr_list_next(elem)) {
            lr_Var *var_val = lr_list_data(elem);
            if (!strcmp(var, var_val->var)) {
                lr_free(var_val->val);
                var_val->val = lr_strdup(value);
                return ret;
            }
        }

        // Add var
        lr_Var *var_val = lr_var_new(var, value);
        ret = lr_list_prepend(urlvars, var_val);
    }

    return ret;
}

void
lr_urlvars_free(lr_UrlVars *urlvars)
{
    for (lr_List *elem = urlvars; elem; elem = lr_list_next(elem))
        lr_var_free(lr_list_data(elem));
    lr_list_free(urlvars);
}

char *
lr_url_substitute(const char *url, lr_UrlVars *vars)
{
    const char *cur = url;
    const char *p = url;
    char *tmp_res, *res;

    if (!url)
        return NULL;

    if (!vars)
        return lr_strdup(url);

    res = lr_strdup("");

    while (*cur != '\0') {
        if (*cur == '$') {
            if (cur-p) {
                char *tmp = lr_strndup(p, cur-p);
                tmp_res = lr_strconcat(res, tmp, NULL);
                lr_free(tmp);
                lr_free(res);
                res = tmp_res;
                p = cur;
            }

            // Try to substitute the variable
            for (lr_List *elem = vars; elem; elem = lr_list_next(elem)) {
                lr_Var *var_val = lr_list_data(elem);
                size_t len = strlen(var_val->var);
                if (!strncmp(var_val->var, (cur+1), len)) {
                    cur = cur + 1 + len;
                    p = cur;
                    tmp_res = lr_strconcat(res, var_val->val, NULL);
                    lr_free(res);
                    res = tmp_res;
                    break;
                }
            }
        }

        ++cur;
    }

    tmp_res = lr_strconcat(res, p, NULL);
    lr_free(res);
    res = tmp_res;

    return res;
}
