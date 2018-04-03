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

#include <glib.h>
#include <glib/gprintf.h>
#include <assert.h>
#include <errno.h>
#include <libxml/parser.h>
#include <unistd.h>
#include "xmlparser.h"
#include "xmlparser_internal.h"
#include "rcodes.h"

LrParserData *
lr_xml_parser_data_new(unsigned int numstates)
{
    LrParserData *pd = g_new0(LrParserData, 1);
    pd->content = g_malloc(CONTENT_REALLOC_STEP);
    pd->acontent = CONTENT_REALLOC_STEP;
    pd->swtab = g_malloc0(sizeof(LrStatesSwitch *) * numstates);
    pd->sbtab = g_malloc(sizeof(unsigned int) * numstates);

    return pd;
}

void
lr_xml_parser_data_free(LrParserData *pd)
{
    if (!pd)
        return;
    g_free(pd->content);
    g_free(pd->swtab);
    g_free(pd->sbtab);
    g_free(pd);
}

void
lr_char_handler(void *pdata, const xmlChar *s, int len)
{
    int l;
    char *c;
    LrParserData *pd = pdata;

    if (pd->err)
        return; /* There was an error -> do nothing */

    if (!pd->docontent)
        return; /* Do not store the content */

    l = pd->lcontent + len + 1;
    if (l > pd->acontent) {
        pd->acontent = l + CONTENT_REALLOC_STEP;
        pd->content = realloc(pd->content, pd->acontent);
    }

    c = pd->content + pd->lcontent;
    pd->lcontent += len;
    while (len-- > 0)
        *c++ = *s++;
    *c = '\0';
}

int
lr_xml_parser_warning(LrParserData *pd,
                      LrXmlParserWarningType type,
                      const char *msg,
                      ...)
{
    int ret;
    va_list args;
    char *warn;
    GError *tmp_err;

    assert(pd);
    assert(msg);

    if (!pd->warningcb)
        return LR_CB_RET_OK;

    va_start(args, msg);
    g_vasprintf(&warn, msg, args);
    va_end(args);

    tmp_err = NULL;
    ret = pd->warningcb(type, warn, pd->warningcb_data, &tmp_err);
    g_free(warn);
    if (ret != LR_CB_RET_OK) {
        if (tmp_err)
            g_propagate_prefixed_error(&pd->err, tmp_err,
                                       "Parsing interrupted: ");
        else
            g_set_error(&pd->err, LR_XML_PARSER_ERROR, LRE_CBINTERRUPTED,
                        "Parsing interrupted by user callback");
    }


    assert(pd->err || ret == LR_CB_RET_OK);

    return ret;
}

gint64
lr_xml_parser_strtoll(LrParserData *pd,
                      const char *nptr,
                      unsigned int base)
{
    gint64 val;
    char *endptr = NULL;

    assert(pd);
    assert(base <= 36 && base != 1);

    if (!nptr)
        return 0;

    val = g_ascii_strtoll(nptr, &endptr, base);

    if ((val == G_MAXINT64 || val == G_MININT64) && errno == ERANGE)
        lr_xml_parser_warning(pd, LR_XML_WARNING_BADATTRVAL,
                "Correct integer value \"%s\" caused overflow", nptr);
    else if (val == 0 && *endptr != '\0')
        lr_xml_parser_warning(pd, LR_XML_WARNING_BADATTRVAL,
                "Conversion of \"%s\" to integer failed", nptr);

    return val;
}

gboolean
lr_xml_parser_generic(XmlParser parser,
                      LrParserData *pd,
                      int fd,
                      GError **err)
{
    /* Note: This function uses .err members of LrParserData! */

    gboolean ret = TRUE;
    xmlParserCtxtPtr ctxt = xmlCreatePushParserCtxt(&parser, pd, NULL, 0, NULL);
    ctxt->linenumbers = 1;

    assert(ctxt);
    assert(pd);
    assert(fd >= 0);
    assert(!err || *err == NULL);

    while (1) {
        int len;
        char buf[XML_BUFFER_SIZE];

        len = read(fd, (void *) buf, XML_BUFFER_SIZE);
        if (len < 0) {
            ret = FALSE;
            g_debug("%s: Error while reading xml : %s\n",
                        __func__, g_strerror(errno));
            g_set_error(err, LR_XML_PARSER_ERROR, LRE_IO,
                        "Error while reading xml: %s", g_strerror(errno));
            break;
        }

        if (xmlParseChunk(ctxt, buf, len, len == 0)) {
            xmlErrorPtr error = xmlCtxtGetLastError(ctxt);
            ret = FALSE;

            g_debug("%s: Parse error at line: %d (%s)",
                        __func__,
                        xmlSAX2GetLineNumber(ctxt),
                        error->message);
            g_set_error(err, LR_XML_PARSER_ERROR, LRE_XMLPARSER,
                        "Parse error at line: %d (%s)",
                        xmlSAX2GetLineNumber(ctxt),
                        error->message);
            break;
        }

        if (pd->err) {
            ret = FALSE;
            g_propagate_error(err, pd->err);
            break;
        }

        if (len == 0)
            break;
    }

    xmlFreeParserCtxt(ctxt);

    return ret;
}
