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

#ifndef __LR_XMLPARSER_INTERNAL_H__
#define __LR_XMLPARSER_INTERNAL_H__

#include <glib.h>
#include <string.h>

#include "repomd.h"
#include "metalink.h"

G_BEGIN_DECLS

/** \defgroup xmlparser_internal Common stuff for XML parsers in Librepo (datatypes, etc.)
 *  \addtogroup xmlparser_internal
 *  @{
 */

#define XML_BUFFER_SIZE         8192
#define CONTENT_REALLOC_STEP    256

typedef xmlSAXHandler XmlParser;

/** Structure used for elements in the state switches in XML parsers
 */
typedef struct {
    unsigned int    from;       /*!< State (current tag) */
    char            *ename;     /*!< String name of sub-tag */
    unsigned int    to;         /*!< State of sub-tag */
    int             docontent;  /*!< Read text content of element? */
} LrStatesSwitch;

/** Parser data
 */
typedef struct {
    int          depth;      /*!< Current depth in a XML tree */
    int          statedepth; /*!< Depth of the last known state (element) */
    unsigned int state;      /*!< current state */
    GError       *err;       /*!< Error message */

    /* Tag content related values */

    int     docontent;  /*!< Store text content of the current element? */
    char    *content;   /*!< Text content of the element */
    int     lcontent;   /*!< The content length */
    int     acontent;   /*!< Available bytes in the content */

    XmlParser      *parser;    /*!< The parser */
    LrStatesSwitch **swtab;    /*!< Pointers to statesswitches table */
    unsigned int    *sbtab;     /*!< stab[to_state] = from_state */

    void *warningcb_data; /*!<
        User data fot he warningcb. */
    LrXmlParserWarningCb warningcb; /*!<
        Warning callback */

    /* Repomd related stuff */

    gboolean repomdfound; /*!<
        True if the <repomd> element was found */
    LrYumRepoMd *repomd; /*!<
        Repomd object */
    LrYumRepoMdRecord *repomdrecord; /*!<
        Repomd record object for a currently parsed element */
    char *cpeid; /*!<
        cpeid value for the currently parsed distro tag */

    /* Metalink related stuff */

    char *filename; /*!<
        filename we are looking for in metalink */
    int ignore; /*!<
        ignore all subelements of the current file element */
    int found; /*!<
        wanted file was already parsed */

    LrMetalink *metalink; /*!<
        metalink object */
    LrMetalinkUrl *metalinkurl; /*!<
        Url in progress or NULL */
    LrMetalinkHash *metalinkhash; /*!<
        Hash in progress or NULL */
    LrMetalinkAlternate *metalinkalternate; /*!<
        Alternate in progress or NULL */

} LrParserData;

/** Malloc and initialize common part of XML parser data.
 */
LrParserData *
lr_xml_parser_data_new(unsigned int numstates);

/** Frees XML parser data
 */
void
lr_xml_parser_data_free(LrParserData *pd);

/** XML character handler
 */
void XMLCALL
lr_char_handler(void *pdata, const xmlChar *s, int len);

/** Find attribute in list of attributes.
 * @param name      Attribute name.
 * @param attr      List of attributes of the tag
 * @return          Value or NULL
 */
static inline const char *
lr_find_attr(const char *name, const char **attr)
{
    /* attr can be NULL when using libxml2 */
    if (!attr)
        return NULL;

    while (*attr) {
        if (!strcmp(name, *attr))
            return attr[1];
        attr += 2;
    }

    return NULL;
}

/** Wrapper for user warning cb.
 * It checks if warningcb is defined, if defined, it build warning msg from
 * va_args, calls warningcb and propagate (set) error if necessary.
 */
int
lr_xml_parser_warning(LrParserData *pd,
                      LrXmlParserWarningType type,
                      const char *msg,
                      ...);

/** strtoll with ability to call warning cb if error during conversion.
 */
gint64
lr_xml_parser_strtoll(LrParserData *pd,
                      const char *nptr,
                      unsigned int base);

/** Generic parser.
 */
gboolean
lr_xml_parser_generic(XmlParser parser,
                      LrParserData *pd,
                      int fd,
                      GError **err);

/** @} */

G_END_DECLS

#endif
