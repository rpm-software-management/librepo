/*
 * Copyright (c) 2012, Tomas Mlcoch
 * Copyright (c) 2007, Novell Inc.
 *
 * This file is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <expat.h>
#include <errno.h>

#include "setup.h"
#include "rcodes.h"
#include "util.h"
#include "repomd.h"

#define CHUNK_SIZE              8192
#define CONTENT_REALLOC_STEP    256

/* Repomd object manipulation helpers */

lr_YumDistroTag
lr_yum_distrotag_init()
{
    return lr_malloc0(sizeof(struct _lr_YumDistroTag));
}

void
lr_yum_distrotag_free(lr_YumDistroTag dt)
{
    if (!dt)
        return;
    lr_free(dt->cpeid);
    lr_free(dt->value);
    lr_free(dt);
}

lr_YumRepoMdRecord
lr_yum_repomdrecord_init()
{
    return lr_malloc0(sizeof(struct _lr_YumRepoMdRecord));
}

void
lr_yum_repomdrecord_free(lr_YumRepoMdRecord rec)
{
    if (!rec)
        return;
    lr_free(rec->type);
    lr_free(rec->location_href);
    lr_free(rec->location_base);
    lr_free(rec->checksum);
    lr_free(rec->checksum_type);
    lr_free(rec->checksum_open);
    lr_free(rec->checksum_open_type);
    lr_free(rec);
}

lr_YumRepoMd
lr_yum_repomd_init()
{
    return lr_malloc0(sizeof(struct _lr_YumRepoMd));
}

void
lr_yum_repomd_clear(lr_YumRepoMd repomd)
{
    if (!repomd)
        return;
    lr_free(repomd->revision);
    for (int x = 0; x < repomd->nort; x++)
        lr_free(repomd->repo_tags[x]);
    lr_free(repomd->repo_tags);
    for (int x = 0; x < repomd->nodt; x++)
        lr_yum_distrotag_free(repomd->distro_tags[x]);
    lr_free(repomd->distro_tags);
    for (int x = 0; x < repomd->noct; x++)
        lr_free(repomd->content_tags[x]);
    lr_free(repomd->content_tags);
    for (int x = 0; x < repomd->nor; x++)
        lr_yum_repomdrecord_free(repomd->records[x]);
    lr_free(repomd->records);
    memset(repomd, 0, sizeof(struct _lr_YumRepoMd));
}

void
lr_yum_repomd_free(lr_YumRepoMd repomd)
{
    if (!repomd)
        return;
    lr_yum_repomd_clear(repomd);
    lr_free(repomd);
}

void
lr_yum_repomd_add_repo_tag(lr_YumRepoMd r_md, char *tag)
{
    assert(r_md);
    r_md->nort++;
    r_md->repo_tags = lr_realloc(r_md->repo_tags, r_md->nort * sizeof(char *));
    r_md->repo_tags[r_md->nort-1] = tag;
}

void
lr_yum_repomd_add_distro_tag(lr_YumRepoMd r_md, lr_YumDistroTag tag)
{
    assert(r_md);
    r_md->nodt++;
    r_md->distro_tags = lr_realloc(r_md->distro_tags,
                                   r_md->nodt * sizeof(lr_YumDistroTag));
    r_md->distro_tags[r_md->nodt-1] = tag;
}

void
lr_yum_repomd_add_content_tag(lr_YumRepoMd r_md, char *tag)
{
    assert(r_md);
    r_md->noct++;
    r_md->content_tags = lr_realloc(r_md->content_tags, r_md->noct * sizeof(char *));
    r_md->content_tags[r_md->noct-1] = tag;
}

/* Idea of parser implementation is borrowed from libsolv */

typedef enum {
    STATE_START,
    STATE_REPOMD,
    STATE_REVISION,
    STATE_TAGS,
    STATE_REPO,
    STATE_CONTENT,
    STATE_DISTRO,
    STATE_DATA,
    STATE_LOCATION,
    STATE_CHECKSUM,
    STATE_OPENCHECKSUM,
    STATE_TIMESTAMP,
    STATE_SIZE,
    STATE_OPENSIZE,
    STATE_DBVERSION,
    NUMSTATES
} lr_State;

typedef struct {
  lr_State from;
  char *ename;
  lr_State to;
  int docontent;
} lr_StatesSwitch;

/* Same states in the first column must be together */
static lr_StatesSwitch stateswitches[] = {
    { STATE_START,      "repomd",           STATE_REPOMD,       0 },
    { STATE_REPOMD,     "revision",         STATE_REVISION,     1 },
    { STATE_REPOMD,     "tags",             STATE_TAGS,         0 },
    { STATE_REPOMD,     "data",             STATE_DATA,         0 },
    { STATE_TAGS,       "repo",             STATE_REPO,         1 },
    { STATE_TAGS,       "content",          STATE_CONTENT,      1 },
    { STATE_TAGS,       "distro",           STATE_DISTRO,       1 },
    { STATE_DATA,       "location",         STATE_LOCATION,     0 },
    { STATE_DATA,       "checksum",         STATE_CHECKSUM,     1 },
    { STATE_DATA,       "open-checksum",    STATE_OPENCHECKSUM, 1 },
    { STATE_DATA,       "timestamp",        STATE_TIMESTAMP,    1 },
    { STATE_DATA,       "size",             STATE_SIZE,         1 },
    { STATE_DATA,       "open-size",        STATE_OPENSIZE,     1 },
    { STATE_DATA,       "database_version", STATE_DBVERSION,    1 },
    { NUMSTATES,        NULL,               NUMSTATES,          0 }
};

typedef struct _ParserData {
    int ret;        /*!< status of parsing (return code) */
    int depth;
    int statedepth;
    lr_State state; /*!< current state */

    int docontent;  /*!< tell if store text from the current element */
    char *content;  /*!< text content of the element */
    int lcontent;   /*!< content lenght */
    int acontent;   /*!< available bytes in the content */

    XML_Parser *parser;                 /*!< parser */
    lr_StatesSwitch *swtab[NUMSTATES];  /*!< pointers to statesswitches table */
    lr_State sbtab[NUMSTATES];          /*!< stab[to_state] = from_state */

    lr_YumRepoMd repomd;            /*!< repomd object */
    lr_YumRepoMdRecord repomd_rec;  /*!< current repomd record */
} ParserData;

static inline const char *
find_attr(const char *txt, const char **atts)
{
    for (; *atts; atts += 2)
        if (!strcmp(*atts, txt))
            return atts[1];
    return 0;
}


static void XMLCALL
start_handler(void *pdata, const char *name, const char **atts)
{
    ParserData *pd = pdata;
    lr_StatesSwitch *sw;

    if (pd->ret != LRE_OK)
        return; /* There was an error -> do nothing */

    if (pd->depth != pd->statedepth) {
        /* There probably was an unknown element */
        pd->depth++;
        return;
    }
    pd->depth++;

    if (!pd->swtab[pd->state])
         return; /* Current element should not have any sub elements */

    /* Find current state by its name */
    for (sw = pd->swtab[pd->state]; sw->from == pd->state; sw++)
        if (!strcmp(name, sw->ename))
            break;
    if (sw->from != pd->state)
      return; /* There is no state for the name -> skip */

    /* Update parser data */
    pd->state = sw->to;
    pd->docontent = sw->docontent;
    pd->statedepth = pd->depth;
    pd->lcontent = 0;
    pd->content[0] = '\0';

    switch(pd->state) {
    case STATE_START:
    case STATE_REPOMD:
    case STATE_REVISION:
    case STATE_TAGS:
    case STATE_REPO:
    case STATE_CONTENT:
        break;

    case STATE_DISTRO: {
        const char *cpeid = find_attr("cpeid", atts);
        lr_YumDistroTag tag = lr_yum_distrotag_init();
        if (cpeid)
            tag->cpeid = lr_strdup(cpeid);
        lr_yum_repomd_add_distro_tag(pd->repomd, tag);
        break;
    }

    case STATE_DATA: {
        const char *type= find_attr("type", atts);
        if (!type) break;
        pd->repomd_rec = lr_yum_repomdrecord_init();
        pd->repomd_rec->type = lr_strdup(type);
        /* Append record to lr_YumRepoMd->records */
        pd->repomd->nor += 1;
        pd->repomd->records = lr_realloc(pd->repomd->records,
                        sizeof(struct _lr_YumRepoMdRecord)*pd->repomd->nor);
        pd->repomd->records[pd->repomd->nor-1] = pd->repomd_rec;
        break;
    }

    case STATE_LOCATION: {
        const char *href = find_attr("href", atts);
        const char *base = find_attr("base", atts);
	if (pd->repomd_rec && href)
            pd->repomd_rec->location_href = lr_strdup(href);
        if (pd->repomd_rec && base)
            pd->repomd_rec->location_base = lr_strdup(base);
        break;
    }

    case STATE_CHECKSUM: {
        const char *type = find_attr("type", atts);
        if (pd->repomd_rec && type)
            pd->repomd_rec->checksum_type = lr_strdup(type);
        break;
    }

    case STATE_OPENCHECKSUM: {
        const char *type= find_attr("type", atts);
	if (pd->repomd_rec && type)
            pd->repomd_rec->checksum_open_type = lr_strdup(type);
        break;
    }

    case STATE_TIMESTAMP:
    case STATE_SIZE:
    case STATE_OPENSIZE:
    case STATE_DBVERSION:
    default:
        break;
    };

    return;
}

static void XMLCALL
char_handler(void *pdata, const XML_Char *s, int len)
{
    int l;
    char *c;
    ParserData *pd = pdata;

    if (pd->ret != LRE_OK)
        return;  /* There was an error -> do nothing */

    if (!pd->docontent)
        return;  /* Do not store the content */

    l = pd->lcontent + len + 1;
    if (l > pd->acontent) {
        pd->acontent = l + CONTENT_REALLOC_STEP;;
        pd->content = lr_realloc(pd->content, pd->acontent);
    }

    c = pd->content + pd->lcontent;
    pd->lcontent += len;
    while (len-- > 0)
        *c++ = *s++;
    *c = '\0';
}

static void XMLCALL
end_handler(void *pdata, const char *name)
{
    ParserData *pd = pdata;

    LR_UNUSED(name);

    if (pd->ret != LRE_OK)
        return;  /* There was an error -> do nothing */

    if (pd->depth != pd->statedepth) {
        /* Back from the unknown state */
        pd->depth--;
        return;
    }

    pd->depth--;
    pd->statedepth--;

    switch (pd->state) {
    case STATE_START:
    case STATE_REPOMD:
        break;

    case STATE_REVISION:
        pd->repomd->revision = lr_strdup(pd->content);
        break;

    case STATE_TAGS:
        break;

    case STATE_REPO:
        lr_yum_repomd_add_repo_tag(pd->repomd, lr_strdup(pd->content));
        break;

    case STATE_CONTENT:
        lr_yum_repomd_add_content_tag(pd->repomd, lr_strdup(pd->content));
        break;

    case STATE_DISTRO:
        if (pd->repomd->nodt < 1) {
            pd->ret = LRE_REPOMDXML;
            break;
        }
        pd->repomd->distro_tags[pd->repomd->nodt-1]->value = lr_strdup(pd->content);
        break;

    case STATE_DATA:
        pd->repomd_rec = NULL;
        break;

    case STATE_LOCATION:
        break;

    case STATE_CHECKSUM:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->checksum = lr_strdup(pd->content);
        break;

    case STATE_OPENCHECKSUM:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->checksum_open = lr_strdup(pd->content);
        break;

    case STATE_TIMESTAMP:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->timestamp = atol(pd->content);
        break;

    case STATE_SIZE:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->size = atol(pd->content);
        break;

    case STATE_OPENSIZE:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->size_open = atol(pd->content);
        break;

    case STATE_DBVERSION:
        if (!pd->repomd_rec)
            break;
        pd->repomd_rec->db_version = atol(pd->content);
        break;

    default:
        break;
    };

    pd->state = pd->sbtab[pd->state];
    pd->docontent = 0;

    return;
}

int
lr_yum_repomd_parse_file(lr_YumRepoMd repomd, int fd)
{
    int ret = LRE_OK;
    XML_Parser parser;
    ParserData pd;
    lr_StatesSwitch *sw;

    assert(repomd);
    DEBUGASSERT(fd >= 0);

    /* Parser configuration */
    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, (void *) &pd);
    XML_SetElementHandler(parser, start_handler, end_handler);
    XML_SetCharacterDataHandler(parser, char_handler);

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
    pd.repomd = repomd;
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
            DPRINTF("%s: Cannot read for parsing : %s\n",
                    __func__, strerror(errno));
            ret = LRE_IO;
            break;
        }

        if (!XML_ParseBuffer(parser, len, len == 0)) {
            DPRINTF("%s: parsing error: %s\n",
                    __func__, XML_ErrorString(XML_GetErrorCode(parser)));
            ret = LRE_REPOMDXML;
            break;
        }

        if (len == 0)
            break;

        if (pd.ret != LRE_OK) {
            ret = pd.ret;
            break;
        }
    }

    /* Parser data cleanup */
    lr_free(pd.content);
    XML_ParserFree(parser);

    return ret;
}

lr_YumRepoMdRecord
lr_yum_repomd_get_record(lr_YumRepoMd repomd, const char *type)
{
    assert(repomd);
    assert(type);
    for (int x=0; x < repomd->nor; x++)
        if (!strcmp(repomd->records[x]->type, type))
            return repomd->records[x];
    return NULL;
}
