/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef __LR_URL_SUBSTITUTION_H__
#define __LR_URL_SUBSTITUTION_H__

#include <glib.h>
#include <stdlib.h>

G_BEGIN_DECLS

/** \defgroup   url_substitution    Substitution of variables in url
 *  \addtogroup url_substitution
 *  @{
 */

/** Element of LrUrlVars list */
typedef struct {
    char *var;  /*!< Variable name (without prefixed $ symbol) */
    char *val;  /*!< Value that will be placed instead of the variable */
} LrVar;

/** LrUrlVars list is in fact GSList */
typedef GSList LrUrlVars;

/** Set value of variable. Use variable names without '$' prefix.
 * If value is NULL, variable will be removed from the list.
 * If list is NULL, new list will be created.
 * @param list          a GSList or NULL for the first item
 * @param var           a variable name (must not be a NULL)
 * @param value         a variable value
 * @return              the new start of the GSList of url substitutions
 */
LrUrlVars *
lr_urlvars_set(LrUrlVars *list, const char *var, const char *value);

/** Frees all of the memory used by LrUrlVars.
 * @param list          a list of substitutions
 */
void
lr_urlvars_free(LrUrlVars *list);

/** Substitute variables in the url. Returns a newly allocated string.
 * @param url           a url
 * @param list          a list of variables and its substitutions or NULL
 * @return              a newly allocated string with substituted url
 */
char *
lr_url_substitute(const char *url, LrUrlVars *list);

/** @} */

G_END_DECLS

#endif
