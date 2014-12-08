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

#ifndef __LR_XMLPARSER_H__
#define __LR_XMLPARSER_H__

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup xmlparser     Common stuff for XML parsers in Librepo (datatypes, etc.)
 *  \addtogroup xmlparser
 *  @{
 */

#define LR_CB_RET_OK    0 /*!< Return value for callbacks signalizing success */
#define LR_CB_RET_ERR   1 /*!< Return value for callbacks signalizing error */

/** Type of warnings reported by parsers by the warning callback.
 */
typedef enum {
    LR_XML_WARNING_UNKNOWNTAG,  /*!< Unknown tag */
    LR_XML_WARNING_MISSINGATTR, /*!< Missing attribute */
    LR_XML_WARNING_UNKNOWNVAL,  /*!< Unknown tag or attribute value */
    LR_XML_WARNING_BADATTRVAL,  /*!< Bad attribute value */
    LR_XML_WARNING_MISSINGVAL,  /*!< Missing tag value */
    LR_XML_WARNING_SENTINEL,
} LrXmlParserWarningType;

/** Callback for XML parser warnings. All reported warnings are non-fatal,
 * and ignored by default. But if callback return LR_CB_RET_ERR instead of
 * LR_CB_RET_OK then parsing is immediately interrupted.
 * @param type      Type of warning
 * @param msg       Warning msg. The message is destroyed after the call.
 *                  If you want touse the message later, you have to copy it.
 * @param cbdata    User data.
 * @param err       GError **
 * @return          LR_CB_RET_OK (0) or LR_CB_RET_ERR (1) - stops the parsing
 */
typedef int (*LrXmlParserWarningCb)(LrXmlParserWarningType type,
                                     char *msg,
                                     void *cbdata,
                                     GError **err);

/** @} */

G_END_DECLS

#endif
