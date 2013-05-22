/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef LR_URL_SUBSTITUTION_H
#define LR_URL_SUBSTITUTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "list.h"

/** \defgroup   url_substitution    Module for substitute vars in url
 *  \addtogroup url_substitution
 *  @{
 */

/** Element of lr_UrlVars list */
typedef struct _lr_Var {
    char *var;
    char *val;
} lr_Var;

/** List with variables and its substitution for url.
 * Each list element has the type lr_Var.
 */
typedef lr_List lr_UrlVars;

/** Set value of variable. Use variable names without '$' prefix.
 * If value is NULL, variable will be removed from the list.
 * If urlvars is NULL, new UrlVars list will be created.
 * @param urlvars       a lr_UrlVars list
 * @param var           a variable name (must not be a NULL)
 * @param value         a variable value
 * @return              the new start of the lr_UrlVars
 */
lr_UrlVars *lr_urlvars_set(lr_UrlVars *urlvars, const char *var, const char *value);

/** Frees all of the memory used by lr_UrlVars.
 * @param urlvars       a lr_UrlVars list
 */
void lr_urlvars_free(lr_UrlVars *urlvars);

/** Substitute variables in the url. Returns a newly allocated string.
 * @param url           a url
 * @param vars          a list of variables and its substitutions
 * @return              a newly allocated string with substituted url
 */
char *lr_url_substitute(const char *url, lr_UrlVars *vars);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
