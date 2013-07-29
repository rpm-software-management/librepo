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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <expat.h>

#include "setup.h"
#include "rcodes.h"
#include "util.h"
#include "metalink.h"

/** TODO:
 * - Better xml parsing error messages
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

static LrMetalinkUrl *
lr_new_metalinkurl(LrMetalink *m)
{
    assert(m);
    LrMetalinkUrl *url = lr_malloc0(sizeof(*url));
    m->urls = g_slist_append(m->urls, url);
    return url;
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

LrMetalink *
lr_metalink_init()
{
    return lr_malloc0(sizeof(LrMetalink));
}

void
lr_metalink_free(LrMetalink *metalink)
{
    if (!metalink)
        return;

    lr_free(metalink->filename);
    g_slist_free_full(metalink->hashes, (GDestroyNotify)lr_free_metalinkhash);
    g_slist_free_full(metalink->urls, (GDestroyNotify)lr_free_metalinkurl);
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
    STATE_RESOURCES,
    STATE_URL,
    NUMSTATES
} LrState;

typedef struct {
    LrState from;  /*!< source state */
    char *ename;    /*!< element name */
    LrState to;    /*!< target state */
    int docontent;  /*!< store the text of the element */
} LrStatesSwitch;

/* Same states in the first column must be together */
static LrStatesSwitch stateswitches[] = {
    { STATE_START,      "metalink",         STATE_METALINK,     0 },
    { STATE_METALINK,   "files",            STATE_FILES,        0 },
    { STATE_FILES,      "file",             STATE_FILE,         0 },
    { STATE_FILE,       "mm0:timestamp",    STATE_TIMESTAMP,    1 },
    { STATE_FILE,       "size",             STATE_SIZE,         1 },
    { STATE_FILE,       "verification",     STATE_VERIFICATION, 0 },
    { STATE_FILE,       "resources",        STATE_RESOURCES,    0 },
    { STATE_VERIFICATION, "hash",           STATE_HASH,         1 },
    { STATE_RESOURCES,  "url",              STATE_URL,          1 },
    { NUMSTATES,        NULL,               NUMSTATES,          0 }
};

typedef struct _ParserData {
    int ret;            /*!< status of parsing (return code) */
    int depth;
    int statedepth;
    LrState state;     /*!< current state */

    int docontent;  /*!< tell if store text from the current element */
    char *content;  /*!< text content of element */
    int lcontent;   /*!< content lenght */
    int acontent;   /*!< availbable bytes in the content */

    XML_Parser *parser;                 /*!< parser */
    LrStatesSwitch *swtab[NUMSTATES];  /*!< pointers to stateswitches table */
    LrState sbtab[NUMSTATES];          /*!< stab[to_state] = from_state */

    char *filename;         /*!< filename we are looking for in metalink */
    int ignore;             /*!< ignore all subelements of the current file element */
    int found;              /*!< wanted file was already parsed */

    LrMetalink *metalink;          /*!< metalink object */
    LrMetalinkUrl *metalinkurl;    /*!< Url in progress or NULL */
    LrMetalinkHash *metalinkhash;  /*!< Hash in progress or NULL */
} ParserData;

static inline const char *
lr_find_attr(const char *name, const char **attr)
{
    while (*attr) {
        if (!strcmp(name, *attr))
            return attr[1];
        attr += 2;
    }

    return NULL;
}

static void XMLCALL
lr_metalink_start_handler(void *pdata, const char *name, const char **attr)
{
    ParserData *pd = pdata;
    LrStatesSwitch *sw;

    if (pd->ret != LRE_OK)
        return; /* There was an error -> do nothing */

    if (pd->depth != pd->statedepth) {
        /* There probably was an unknown element */
        pd->depth++;
        return;
    }
    pd->depth++;

    if (!pd->swtab[pd->state])
        return;  /* Current element should not have any sub elements */

    /* Find current state by its name */
    for (sw = pd->swtab[pd->state]; sw->from == pd->state; sw++)
        if (!strcmp(name, sw->ename))
            break;
    if (sw->from != pd->state)
        return;  /* There is no state for the name -> skip */

    /* Update parser data */
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
        const char *name = lr_find_attr("name", attr);
        if (!name) {
            g_debug("%s: file element doesn't have name attribute", __func__);
            pd->ret = LRE_MLXML;
            break;
        }
        if (pd->found || strcmp(name, pd->filename)) {
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
        break;

    case STATE_HASH: {
        LrMetalinkHash *mh;
        assert(!pd->metalinkhash);
        const char *type = lr_find_attr("type", attr);
        if (!type) {
            g_debug("%s: hash element doesn't have type attribute", __func__);
            pd->ret = LRE_MLXML;
            break;
        }
        mh = lr_new_metalinkhash(pd->metalink);
        mh->type = g_strdup(type);
        pd->metalinkhash = mh;
        break;
    }

    case STATE_RESOURCES:
        break;

    case STATE_URL: {
        const char *val;
        assert(!pd->metalinkurl);
        LrMetalinkUrl *url = lr_new_metalinkurl(pd->metalink);
        if ((val = lr_find_attr("protocol", attr)))
            url->protocol = g_strdup(val);
        if ((val = lr_find_attr("type", attr)))
            url->type = g_strdup(val);
        if ((val = lr_find_attr("location", attr)))
            url->location = g_strdup(val);
        if ((val = lr_find_attr("preference", attr)))
            url->preference = atol(val);
        pd->metalinkurl = url;
        break;
    }

    default:
        break;
    };

    return;
}

static void XMLCALL
lr_metalink_char_handler(void *pdata, const XML_Char *s, int len)
{
    int l;
    char *c;
    ParserData *pd = pdata;

    if (pd->ret != LRE_OK)
        return; /* There was an error -> do nothing */

    if (!pd->docontent)
        return; /* Do not store the content */

    if (pd->ignore)
        return; /* Ignore all content */

    l = pd->lcontent + len + 1;
    if (l > pd->acontent) {
        pd->acontent = l + CONTENT_REALLOC_STEP;
        pd->content = lr_realloc(pd->content, pd->acontent);
    }

    c = pd->content + pd->lcontent;
    pd->lcontent += len;
    while (len-- > 0)
        *c++ = *s++;
    *c = '\0';
}

static void XMLCALL
lr_metalink_end_handler(void *pdata, const char *name)
{
    ParserData *pd = pdata;

    LR_UNUSED(name);

    if (pd->ret != LRE_OK)
        return; /* There was an error -> do nothing */

    if (pd->depth != pd->statedepth) {
        /* Back from the unknown state */
        pd->depth--;
        return;
    }

    pd->depth--;
    pd->statedepth--;

    if (pd->ignore && pd->state != STATE_FILE) {
        pd->state = pd->sbtab[pd->state];
        pd->docontent = 0;
        return; /* Ignore all subelements of the current file element */
    }

    switch (pd->state) {
    case STATE_START:
    case STATE_METALINK:
    case STATE_FILES:
    case STATE_FILE:
    case STATE_VERIFICATION:
    case STATE_RESOURCES:
        break;

    case STATE_TIMESTAMP:
        pd->metalink->timestamp = atol(pd->content);
        break;

    case STATE_SIZE:
        pd->metalink->size = atol(pd->content);
        break;

    case STATE_HASH:
        assert(pd->metalinkhash);
        pd->metalinkhash->value = g_strdup(pd->content);
        pd->metalinkhash = NULL;
        break;

    case STATE_URL:
        assert(pd->metalinkurl);
        pd->metalinkurl->url = g_strdup(pd->content);
        pd->metalinkurl = NULL;
        break;

    default:
        break;
    };

    pd->state = pd->sbtab[pd->state];
    pd->docontent = 0;

    return;
}

int
lr_metalink_parse_file(LrMetalink *metalink,
                       int fd,
                       const char *filename,
                       GError **err)
{
    int ret = LRE_OK;
    XML_Parser parser;
    ParserData pd;
    LrStatesSwitch *sw;

    assert(metalink);
    assert(fd >= 0);
    assert(filename);
    assert(!err || *err == NULL);

    /* Parser configuration */
    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, (void *) &pd);
    XML_SetElementHandler(parser, lr_metalink_start_handler, lr_metalink_end_handler);
    XML_SetCharacterDataHandler(parser, lr_metalink_char_handler);

    /* Initialization of parser data */
    memset(&pd, 0, sizeof(pd));
    pd.ret = LRE_OK;
    pd.depth = 0;
    pd.state = STATE_START;
    pd.statedepth = 0;
    pd.docontent = 0;
    pd.content = lr_malloc(CONTENT_REALLOC_STEP);
    pd.lcontent = 0;
    pd.acontent = CONTENT_REALLOC_STEP;
    pd.parser = &parser;
    pd.metalink = metalink;
    pd.filename = (char *) filename;
    pd.ignore = 1;
    pd.found = 0;
    for (sw = stateswitches; sw->from != NUMSTATES; sw++) {
        if (!pd.swtab[sw->from])
            pd.swtab[sw->from] = sw;
        pd.sbtab[sw->to] = sw->from;
    }

    /* Parse */
    for (;;) {
        char *buf;
        int len;

        buf = XML_GetBuffer(parser, CHUNK_SIZE);
        if (!buf)
            lr_out_of_memory();

        len = read(fd, (void *) buf, CHUNK_SIZE);
        if (len < 0) {
            g_debug("%s: Cannot read for parsing : %s",
                    __func__, strerror(errno));
            g_set_error(err, LR_METALINK_ERROR, LRE_IO,
                        "Cannot read metalink fd %d: %s", fd, strerror(errno));
            ret = LRE_IO;
            break;
        }

        if (!XML_ParseBuffer(parser, len, len == 0)) {
            g_debug("%s: parsing error: %s",
                    __func__, XML_ErrorString(XML_GetErrorCode(parser)));
            g_set_error(err, LR_METALINK_ERROR, LRE_MLXML,
                        "Metalink parser error: %s",
                        XML_ErrorString(XML_GetErrorCode(parser)));
            ret = LRE_MLXML;
            break;
        }

        if (len == 0)
            break;

        if (pd.ret != LRE_OK) {
            ret = pd.ret;
            g_set_error(err, LR_METALINK_ERROR, ret,
                        "Error while parsing metalink: %s", lr_strerror(ret));
            break;
        }
    }

    /* Parser data cleanup */
    lr_free(pd.content);
    XML_ParserFree(parser);

    if (!pd.found) {
        g_set_error(err, LR_METALINK_ERROR, LRE_MLBAD,
                    "Bad metalink, file %s was not found", filename);
        return LRE_MLBAD; /* The wanted file was not found in metalink */
    }

    return ret;
}
