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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <errno.h>

#include "repomd.h"
#include "xmlparser_internal.h"
#include "rcodes.h"
#include "util.h"

#define CHUNK_SIZE              8192
#define CONTENT_REALLOC_STEP    256

/* Repomd object manipulation helpers */

static LrYumRepoMdRecord *
lr_yum_repomdrecord_init(const char *type)
{
    LrYumRepoMdRecord *record = lr_malloc0(sizeof(*record));
    record->chunk = g_string_chunk_new(128);
    record->type = lr_string_chunk_insert(record->chunk, type);
    return record;
}

static void
lr_yum_repomdrecord_free(LrYumRepoMdRecord *rec)
{
    if (!rec)
        return;
    g_string_chunk_free(rec->chunk);
    lr_free(rec);
}

LrYumRepoMd *
lr_yum_repomd_init(void)
{
    LrYumRepoMd *repomd = lr_malloc0(sizeof(*repomd));
    repomd->chunk = g_string_chunk_new(32);
    return repomd;
}

void
lr_yum_repomd_free(LrYumRepoMd *repomd)
{
    if (!repomd)
        return;
    g_slist_free_full(repomd->records, (GDestroyNotify) lr_yum_repomdrecord_free);
    g_slist_free(repomd->repo_tags);
    g_slist_free(repomd->content_tags);
    g_slist_free_full(repomd->distro_tags, (GDestroyNotify) g_free);
    g_string_chunk_free(repomd->chunk);
    g_free(repomd);
}

static void
lr_yum_repomd_set_record(LrYumRepoMd *repomd,
                         LrYumRepoMdRecord *record)
{
    if (!repomd || !record) return;
    repomd->records = g_slist_append(repomd->records, record);
}

static void
lr_yum_repomd_set_revision(LrYumRepoMd *repomd, const char *revision)
{
    if (!repomd) return;
    repomd->revision = lr_string_chunk_insert(repomd->chunk, revision);
}

static void
lr_yum_repomd_add_repo_tag(LrYumRepoMd *repomd, char *tag)
{
    assert(repomd);
    if (!tag) return;
    repomd->repo_tags = g_slist_append(repomd->repo_tags,
                                g_string_chunk_insert(repomd->chunk, tag));
}

static void
lr_yum_repomd_add_content_tag(LrYumRepoMd *repomd, char *tag)
{
    assert(repomd);
    if (!tag) return;
    repomd->content_tags = g_slist_append(repomd->content_tags,
                                g_string_chunk_insert(repomd->chunk, tag));
}

static void
lr_yum_repomd_add_distro_tag(LrYumRepoMd *repomd,
                             const char *cpeid,
                             const char *tag)
{
    assert(repomd);
    if (!tag) return;

    LrYumDistroTag *distrotag = lr_malloc0(sizeof(*distrotag));
    distrotag->cpeid = lr_string_chunk_insert(repomd->chunk, cpeid);
    distrotag->tag   = g_string_chunk_insert(repomd->chunk, tag);
    repomd->distro_tags = g_slist_append(repomd->distro_tags, distrotag);
}

LrYumRepoMdRecord *
lr_yum_repomd_get_record(LrYumRepoMd *repomd, const char *type)
{
    assert(repomd);
    assert(type);

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        LrYumRepoMdRecord *record = elem->data;
        assert(record);
        if (!g_strcmp0(record->type, type))
            return record;
    }
    return NULL;
}

gint64
lr_yum_repomd_get_highest_timestamp(LrYumRepoMd *repomd, GError **err)
{
    gint64 max = 0;

    assert(repomd);
    assert(!err || *err == NULL);

    if (!repomd->records) {
        g_set_error(err, LR_REPOMD_ERROR, LRE_REPOMD,
                    "repomd.xml has no records");
        return max;
    }

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        LrYumRepoMdRecord *record = elem->data;
        assert(record);
        if (max < record->timestamp)
            max = record->timestamp;
    }

    return max;
}

// repomd.xml parser

typedef enum {
    STATE_START,
    STATE_REPOMD,
    STATE_REVISION,
    STATE_REPOID,
    STATE_TAGS,
    STATE_REPO,
    STATE_CONTENT,
    STATE_DISTRO,
    STATE_DATA,
    STATE_LOCATION,
    STATE_CHECKSUM,
    STATE_OPENCHECKSUM,
    STATE_HEADERCHECKSUM,
    STATE_TIMESTAMP,
    STATE_SIZE,
    STATE_OPENSIZE,
    STATE_HEADERSIZE,
    STATE_DBVERSION,
    NUMSTATES
} LrRepomdState;

/* NOTE: Same states in the first column must be together!!!
* Performance tip: More frequent elements shoud be listed
* first in its group (eg: element "package" (STATE_PACKAGE)
* has a "file" element listed first, because it is more frequent
* than a "version" element). */
static LrStatesSwitch stateswitches[] = {
    { STATE_START,      "repomd",              STATE_REPOMD,         0 },
    { STATE_REPOMD,     "revision",            STATE_REVISION,       1 },
    { STATE_REPOMD,     "repoid",              STATE_REPOID,         1 },
    { STATE_REPOMD,     "tags",                STATE_TAGS,           0 },
    { STATE_REPOMD,     "data",                STATE_DATA,           0 },
    { STATE_TAGS,       "repo",                STATE_REPO,           1 },
    { STATE_TAGS,       "content",             STATE_CONTENT,        1 },
    { STATE_TAGS,       "distro",              STATE_DISTRO,         1 },
    { STATE_DATA,       "location",            STATE_LOCATION,       0 },
    { STATE_DATA,       "checksum",            STATE_CHECKSUM,       1 },
    { STATE_DATA,       "open-checksum",       STATE_OPENCHECKSUM,   1 },
    { STATE_DATA,       "header-checksum",     STATE_HEADERCHECKSUM, 1 },
    { STATE_DATA,       "timestamp",           STATE_TIMESTAMP,      1 },
    { STATE_DATA,       "size",                STATE_SIZE,           1 },
    { STATE_DATA,       "open-size",           STATE_OPENSIZE,       1 },
    { STATE_DATA,       "header-size",         STATE_HEADERSIZE,     1 },
    { STATE_DATA,       "database_version",    STATE_DBVERSION,      1 },
    { NUMSTATES,        NULL,                  NUMSTATES,            0 }
};

static void
lr_start_handler(void *pdata, const xmlChar *xmlElement, const xmlChar **xmlAttr)
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
        if (!strcmp(element, sw->ename))
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

    const char *val;

    switch(pd->state) {
    case STATE_START:
        break;

    case STATE_REPOMD:
        pd->repomdfound = TRUE;
        break;

    case STATE_REVISION:
    case STATE_TAGS:
    case STATE_REPO:
    case STATE_CONTENT:
        break;

    case STATE_REPOID:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        val = lr_find_attr("type", attr);
        if (val)
            pd->repomd->repoid_type = g_string_chunk_insert(pd->repomd->chunk,
                                                            val);
        break;

    case STATE_DISTRO:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        val = lr_find_attr("cpeid", attr);
        if (val)
            pd->cpeid = g_strdup(val);
        break;

    case STATE_DATA:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        val = lr_find_attr("type", attr);
        if (!val) {
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                           "Missing attribute \"type\" of a data element");
            val = "unknown";
        }

        pd->repomdrecord = lr_yum_repomdrecord_init(val);
        lr_yum_repomd_set_record(pd->repomd, pd->repomdrecord);
        break;

    case STATE_LOCATION:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        val = lr_find_attr("href", attr);
        if (val)
            pd->repomdrecord->location_href = g_string_chunk_insert(
                                                    pd->repomdrecord->chunk,
                                                    val);
        else
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                    "Missing attribute \"href\" of a location element");

        val = lr_find_attr("xml:base", attr);
        if (val)
            pd->repomdrecord->location_base = g_string_chunk_insert(
                                                    pd->repomdrecord->chunk,
                                                    val);

        break;

    case STATE_CHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        val = lr_find_attr("type", attr);
        if (!val) {
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                    "Missing attribute \"type\" of a checksum element");
            break;
        }

        pd->repomdrecord->checksum_type = g_string_chunk_insert(
                                                    pd->repomdrecord->chunk,
                                                    val);
        break;

    case STATE_OPENCHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        val = lr_find_attr("type", attr);
        if (!val) {
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                    "Missing attribute \"type\" of an open checksum element");
            break;
        }

        pd->repomdrecord->checksum_open_type = g_string_chunk_insert(
                                                    pd->repomdrecord->chunk,
                                                    val);
        break;

    case STATE_HEADERCHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        val = lr_find_attr("type", attr);
        if (!val) {
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGATTR,
                    "Missing attribute \"type\" of an open checksum element");
            break;
        }

        pd->repomdrecord->header_checksum_type = g_string_chunk_insert(
                                                     pd->repomdrecord->chunk,
                                                     val);
        break;

    case STATE_TIMESTAMP:
    case STATE_SIZE:
    case STATE_OPENSIZE:
    case STATE_HEADERSIZE:
    case STATE_DBVERSION:
    default:
        break;
    }
}

static void
lr_end_handler(void *pdata, G_GNUC_UNUSED const xmlChar *element)
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

    switch (state) {
    case STATE_START:
    case STATE_REPOMD:
        break;

    case STATE_REVISION:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        if (pd->lcontent == 0) {
            lr_xml_parser_warning(pd, LR_XML_WARNING_MISSINGVAL,
                    "Missing value of a revision element");
            break;
        }

        lr_yum_repomd_set_revision(pd->repomd, pd->content);
        break;

    case STATE_REPOID:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        pd->repomd->repoid = g_string_chunk_insert(pd->repomd->chunk,
                                                   pd->content);
        break;

    case STATE_TAGS:
        break;

    case STATE_REPO:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        lr_yum_repomd_add_repo_tag(pd->repomd, pd->content);
        break;

    case STATE_CONTENT:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        lr_yum_repomd_add_content_tag(pd->repomd, pd->content);
        break;

    case STATE_DISTRO:
        assert(pd->repomd);
        assert(!pd->repomdrecord);

        lr_yum_repomd_add_distro_tag(pd->repomd, pd->cpeid, pd->content);
        if (pd->cpeid) {
            g_free(pd->cpeid);
            pd->cpeid = NULL;
        }
        break;

    case STATE_DATA:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord = NULL;
        break;

    case STATE_LOCATION:
        break;

    case STATE_CHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->checksum = lr_string_chunk_insert(
                                            pd->repomdrecord->chunk,
                                            pd->content);
        break;

    case STATE_OPENCHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->checksum_open = lr_string_chunk_insert(
                                            pd->repomdrecord->chunk,
                                            pd->content);
        break;

    case STATE_HEADERCHECKSUM:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->header_checksum = lr_string_chunk_insert(
                                                pd->repomdrecord->chunk,
                                                pd->content);
        break;

    case STATE_TIMESTAMP:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->timestamp = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_SIZE:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->size = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_OPENSIZE:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->size_open = lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    case STATE_HEADERSIZE:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->size_header = lr_xml_parser_strtoll(pd, pd->content,
                                                              0);
        break;

    case STATE_DBVERSION:
        assert(pd->repomd);
        assert(pd->repomdrecord);

        pd->repomdrecord->db_version = (int) lr_xml_parser_strtoll(pd, pd->content, 0);
        break;

    default:
        break;
    }
}

gboolean
lr_yum_repomd_parse_file(LrYumRepoMd *repomd,
                         int fd,
                         LrXmlParserWarningCb warningcb,
                         void *warningcb_data,
                         GError **err)
{
    gboolean ret = TRUE;
    LrParserData *pd;
    XmlParser parser;
    GError *tmp_err = NULL;

    assert(fd >= 0);
    assert(repomd);
    assert(!err || *err == NULL);

    // Init

    memset(&parser, 0, sizeof(parser));
    parser.startElement = lr_start_handler;
    parser.endElement = lr_end_handler;
    parser.characters = lr_char_handler;

    pd = lr_xml_parser_data_new(NUMSTATES);
    pd->parser = &parser;
    pd->state = STATE_START;
    pd->repomd = repomd;
    pd->warningcb = warningcb;
    pd->warningcb_data = warningcb_data;
    for (LrStatesSwitch *sw = stateswitches; sw->from != NUMSTATES; sw++) {
        if (!pd->swtab[sw->from])
            pd->swtab[sw->from] = sw;
        pd->sbtab[sw->to] = sw->from;
    }

    // Parsing

    ret = lr_xml_parser_generic(parser, pd, fd, &tmp_err);
    if (tmp_err)
        g_propagate_error(err, tmp_err);

    // Check of results

    if (!tmp_err && !pd->repomdfound) {
        ret = FALSE;
        g_set_error(err, LR_XML_PARSER_ERROR, LRE_REPOMDXML,
                    "Element <repomd> was not found - Bad repomd file");
    }

    // Clean up

    lr_xml_parser_data_free(pd);

    return ret;
}
