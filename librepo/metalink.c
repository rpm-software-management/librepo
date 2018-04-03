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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libxml/parser.h>

#include "rcodes.h"
#include "util.h"
#include "metalink.h"
#include "xmlparser_internal.h"

/** TODO:
 * - (?) Use GStringChunk
 */

#define CHUNK_SIZE              8192
#define CONTENT_REALLOC_STEP    256

/* Metalink object manipulation helpers */

static LrMetalinkHash *
lr_new_metalinkhash(LrMetalink *m)
{
    assert(m);
    LrMetalinkHash *hash = lr_malloc0(sizeof(*hash));
    m->hashes = g_slist_append(m->hashes, hash);
    return hash;
}

static LrMetalinkHash *
lr_new_metalinkalternate_hash(LrMetalinkAlternate *ma)
{
    assert(ma);
    LrMetalinkHash *hash = lr_malloc0(sizeof(*hash));
    ma->hashes = g_slist_append(ma->hashes, hash);
    return hash;
}

static LrMetalinkUrl *
lr_new_metalinkurl(LrMetalink *m)
{
    assert(m);
    LrMetalinkUrl *url = lr_malloc0(sizeof(*url));
    m->urls = g_slist_append(m->urls, url);
    return url;
}

static LrMetalinkAlternate *
lr_new_metalinkalternate(LrMetalink *m)
{
    assert(m);
    LrMetalinkAlternate *alternate = lr_malloc0(sizeof(*alternate));
    m->alternates = g_slist_append(m->alternates, alternate);
    return alternate;
}

static void
lr_free_metalinkhash(LrMetalinkHash *metalinkhash)
{
    if (!metalinkhash) return;
    lr_free(metalinkhash->type);
    lr_free(metalinkhash->value);
    lr_free(metalinkhash);
}

static void
lr_free_metalinkurl(LrMetalinkUrl *metalinkurl)
{
    if (!metalinkurl) return;
    lr_free(metalinkurl->protocol);
    lr_free(metalinkurl->type);
    lr_free(metalinkurl->location);
    lr_free(metalinkurl->url);
    lr_free(metalinkurl);
}

static void
lr_free_metalinkalternate(LrMetalinkAlternate *metalinkalternate)
{
    if (!metalinkalternate) return;
    g_slist_free_full(metalinkalternate->hashes,
                      (GDestroyNotify)lr_free_metalinkhash);
    lr_free(metalinkalternate);
}

LrMetalink *
lr_metalink_init(void)
{
    return lr_malloc0(sizeof(LrMetalink));
}

void
lr_metalink_free(LrMetalink *metalink)
{
    if (!metalink)
        return;

    lr_free(metalink->filename);
    g_slist_free_full(metalink->hashes,
                      (GDestroyNotify)lr_free_metalinkhash);
    g_slist_free_full(metalink->urls,
                      (GDestroyNotify)lr_free_metalinkurl);
    g_slist_free_full(metalink->alternates,
                      (GDestroyNotify)lr_free_metalinkalternate);
    lr_free(metalink);
}

/* Idea of parser implementation is borrowed from libsolv */

typedef enum {
    STATE_START,
    STATE_METALINK,
    STATE_FILES,
    STATE_FILE,
    STATE_TIMESTAMP,
    STATE_SIZE,
    STATE_VERIFICATION,
    STATE_HASH,
    STATE_ALTERNATES,
    STATE_ALTERNATE,
    STATE_ALTERNATE_TIMESTAMP,
    STATE_ALTERNATE_SIZE,
    STATE_ALTERNATE_VERIFICATION,
    STATE_ALTERNATE_HASH,
    STATE_RESOURCES,
    STATE_URL,
    NUMSTATES
} LrState;

/* Same states in the first column must be together */
static LrStatesSwitch stateswitches[] = {
    { STATE_START,      "metalink",         STATE_METALINK,                0 },
    { STATE_METALINK,   "files",            STATE_FILES,                   0 },
    { STATE_FILES,      "file",             STATE_FILE,                    0 },
    { STATE_FILE,       "mm0:timestamp",    STATE_TIMESTAMP,               1 },
    { STATE_FILE,       "size",             STATE_SIZE,                    1 },
    { STATE_FILE,       "verification",     STATE_VERIFICATION,            0 },
    { STATE_FILE,       "mm0:alternates",   STATE_ALTERNATES,              0 },
    { STATE_FILE,       "resources",        STATE_RESOURCES,               0 },
    { STATE_VERIFICATION, "hash",           STATE_HASH,                    1 },
    { STATE_ALTERNATES, "mm0:alternate",    STATE_ALTERNATE,               0 },
    { STATE_ALTERNATE,  "mm0:timestamp",    STATE_ALTERNATE_TIMESTAMP,     1 },
    { STATE_ALTERNATE,  "size",             STATE_ALTERNATE_SIZE,          1 },
    { STATE_ALTERNATE,  "verification",     STATE_ALTERNATE_VERIFICATION,  0 },
    { STATE_ALTERNATE_VERIFICATION, "hash", STATE_ALTERNATE_HASH,          1 },
    { STATE_RESOURCES,  "url",              STATE_URL,                     1 },
    { NUMSTATES,        NULL,               NUMSTATES,                     0 }
};

static void
lr_metalink_start_handler(void *pdata, const xmlChar *xmlElement, const xmlChar **xmlAttr)
{
    LrParserData *pd = pdata;
    LrStatesSwitch *sw;
    const char **attr = (const char **)xmlAttr;
    const char *element = (const char *)xmlElement;

    if (pd->err)
        return; // There was an error -> do nothing

    if (pd->depth != pd->statedepth) {
        // We are inside of unknown element
        pd->depth++;
        return;
    }
    pd->depth++;

    if (!pd->swtab[pd->state]) {
        // Current element should not have any sub elements
        return;
    }

    // Find current state by its name
    for (sw = pd->swtab[pd->state]; sw->from == pd->state; sw++)
        if (!g_strcmp0(element, sw->ename))
            break;
    if (sw->from != pd->state) {
        // No state for current element (unknown element)
        lr_xml_parser_warning(pd, LR_XML_WARNING_UNKNOWNTAG,
                              "Unknown element \"%s\"", element);
        return;
    }

    // Update parser data
    pd->state = sw->to;
    pd->docontent = sw->docontent;
    pd->statedepth = pd->depth;
    pd->lcontent = 0;
    pd->content[0] = '\0';

    if (pd->ignore && pd->state != STATE_FILE)
        return; /* Ignore all subelements of the current file element */

    switch (pd->state) {
    case STATE_START:
    case STATE_METALINK:
    case STATE_FILES:
        break;

    case STATE_FILE: {
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);

        const char *name = lr_find_attr("name", attr);
        if (!name) {
            g_debug("%s: Missing attribute \"name\" of file element", __func__);
            g_set_error(&pd->err, LR_METALINK_ERROR, LRE_MLXML,
                        "Missing attribute \"name\" of file element");
            break;
        }
        if (pd->found || g_strcmp0(name, pd->filename)) {
            pd->ignore = 1;
            break;
        } else {
            pd->ignore = 0;
            pd->found = 1;
        }
        pd->metalink->filename = g_strdup(name);
        break;
    }
    case STATE_TIMESTAMP:
    case STATE_SIZE:
    case STATE_VERIFICATION:
    case STATE_ALTERNATES:
        break;

    case STATE_ALTERNATE:
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);
        assert(!pd->metalinkalternate);

        LrMetalinkAlternate *ma;
        ma = lr_new_metalinkalternate(pd->metalink);
        pd->metalinkalternate = ma;
        break;

    case STATE_ALTERNATE_TIMESTAMP:
    case STATE_ALTERNATE_SIZE:
    case STATE_ALTERNATE_VERIFICATION:
        break;

    case STATE_HASH: {
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);
        assert(!pd->metalinkalternate);

        LrMetalinkHash *mh;
        const char *type = lr_find_attr("type", attr);
        if (!type) {
            // Type of the hash is not specifed -> skip it
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                              "hash element doesn't have attribute \"type\"");
            break;
        }
        mh = lr_new_metalinkhash(pd->metalink);
        mh->type = g_strdup(type);
        pd->metalinkhash = mh;
        break;
    }

    case STATE_ALTERNATE_HASH: {
        assert(pd->metalink);
        assert(pd->metalinkalternate);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);

        LrMetalinkHash *mh;
        const char *type = lr_find_attr("type", attr);
        if (!type) {
            // Type of the hash is not specifed -> skip it
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                              "hash element doesn't have attribute \"type\"");
            break;
        }
        mh = lr_new_metalinkalternate_hash(pd->metalinkalternate);
        mh->type = g_strdup(type);
        pd->metalinkhash = mh;
        break;
    }

    case STATE_RESOURCES:
        break;

    case STATE_URL: {
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);

        const char *val;
        assert(!pd->metalinkurl);
        LrMetalinkUrl *url = lr_new_metalinkurl(pd->metalink);
        if ((val = lr_find_attr("protocol", attr)))
            url->protocol = g_strdup(val);
        if ((val = lr_find_attr("type", attr)))
            url->type = g_strdup(val);
        if ((val = lr_find_attr("location", attr)))
            url->location = g_strdup(val);
        if ((val = lr_find_attr("preference", attr))) {
            long long ll_val = lr_xml_parser_strtoll(pd, val, 0);
            if (ll_val < 0 || ll_val > 100) {
                lr_xml_parser_warning(pd, LR_XML_WARNING_BADATTRVAL,
                "Bad value (\"%s\") of \"preference\" attribute in url element"
                " (should be in range 0-100)", val);
            } else {
                url->preference = ll_val;
            }
        }
        pd->metalinkurl = url;
        break;
    }

    default:
        break;
    };

    return;
}

static void
lr_metalink_end_handler(void *pdata, G_GNUC_UNUSED const xmlChar *element)
{
    LrParserData *pd = pdata;
    unsigned int state = pd->state;

    if (pd->err)
        return; // There was an error -> do nothing

    if (pd->depth != pd->statedepth) {
        // Back from the unknown state
        pd->depth--;
        return;
    }

    pd->depth--;
    pd->statedepth--;
    pd->state = pd->sbtab[pd->state];
    pd->docontent = 0;

    if (pd->ignore && state != STATE_FILE) {
        // Ignore all subelements of the current file element
        return;
    }

    switch (state) {
    case STATE_START:
    case STATE_METALINK:
    case STATE_FILES:
    case STATE_FILE:
    case STATE_VERIFICATION:
    case STATE_ALTERNATES:
    case STATE_ALTERNATE_VERIFICATION:
        break;

    case STATE_RESOURCES:
        break;

    case STATE_TIMESTAMP:
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);

        pd->metalink->timestamp = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_SIZE:
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);

        pd->metalink->size = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_HASH:
        assert(pd->metalink);
        assert(!pd->metalinkurl);

        if (!pd->metalinkhash) {
            // If hash has no type
            break;
        }

        pd->metalinkhash->value = g_strdup(pd->content);
        pd->metalinkhash = NULL;
        break;

    case STATE_ALTERNATE:
        assert(pd->metalink);
        assert(pd->metalinkalternate);
        pd->metalinkalternate = NULL;
        break;

    case STATE_ALTERNATE_TIMESTAMP:
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);
        assert(pd->metalinkalternate);

        pd->metalinkalternate->timestamp = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_ALTERNATE_SIZE:
        assert(pd->metalink);
        assert(!pd->metalinkurl);
        assert(!pd->metalinkhash);
        assert(pd->metalinkalternate);

        pd->metalinkalternate->size = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_ALTERNATE_HASH:
        assert(pd->metalink);
        assert(pd->metalinkalternate);
        assert(!pd->metalinkurl);

        if (!pd->metalinkhash) {
            // If hash has no type
            break;
        }

        pd->metalinkhash->value = g_strdup(pd->content);
        pd->metalinkhash = NULL;
        break;

    case STATE_URL:
        assert(pd->metalink);
        assert(pd->metalinkurl);
        assert(!pd->metalinkhash);

        pd->metalinkurl->url = g_strdup(pd->content);
        pd->metalinkurl = NULL;
        break;

    default:
        break;
    };

    return;
}

gboolean
lr_metalink_parse_file(LrMetalink *metalink,
                       int fd,
                       const char *filename,
                       LrXmlParserWarningCb warningcb,
                       void *warningcb_data,
                       GError **err)
{
    gboolean ret = TRUE;
    LrParserData *pd;
    XmlParser parser;
    GError *tmp_err = NULL;

    assert(metalink);
    assert(fd >= 0);
    assert(filename);
    assert(!err || *err == NULL);

    // Init

    memset(&parser, 0, sizeof(parser));
    parser.startElement = lr_metalink_start_handler;
    parser.endElement = lr_metalink_end_handler;
    parser.characters = lr_char_handler;

    pd = lr_xml_parser_data_new(NUMSTATES);
    pd->parser = &parser;
    pd->state = STATE_START;
    pd->metalink = metalink;
    pd->filename = (char *) filename;
    pd->ignore = 1;
    pd->found = 0;
    pd->warningcb = warningcb;
    pd->warningcb_data = warningcb_data;
    for (LrStatesSwitch *sw = stateswitches; sw->from != NUMSTATES; sw++) {
        if (!pd->swtab[sw->from])
            pd->swtab[sw->from] = sw;
        pd->sbtab[sw->to] = sw->from;
    }

    // Parsing

    ret = lr_xml_parser_generic(parser, pd, fd, &tmp_err);
    if (tmp_err) {
        g_propagate_error(err, tmp_err);
        goto err;
    }

    // Clean up

    if (!pd->found) {
        g_set_error(err, LR_METALINK_ERROR, LRE_MLBAD,
                    "file \"%s\" was not found in metalink", filename);
        ret = FALSE; // The wanted file was not found in metalink
    }

err:
    lr_xml_parser_data_free(pd);

    return ret;
}
