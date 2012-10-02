#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <expat.h>

#include "setup.h"
#include "librepo.h"
#include "util.h"
#include "metalink.h"

#define CHUNK_SIZE              8192
#define HASHES_REALLOC_STEP     5
#define URLS_REALLOC_STEP       10
#define CONTENT_REALLOC_STEP    256

/* Metalink object manipulation helpers */

lr_MetalinkHash
lr_new_metalinkhash(lr_Metalink m)
{
    assert(m);

    if (m->noh+1 > m->loh) {
        m->loh += HASHES_REALLOC_STEP;
        m->hashes = lr_realloc(m->hashes, m->loh * sizeof(lr_MetalinkHash));
    }

    m->hashes[m->noh] = lr_malloc0(sizeof(struct _lr_MetalinkHash));
    m->noh++;
    return m->hashes[m->noh-1];
}

lr_MetalinkUrl
lr_new_metalinkurl(lr_Metalink m)
{
    assert(m);

    if (m->nou+1 > m->lou) {
        m->lou += URLS_REALLOC_STEP;
        m->urls = lr_realloc(m->urls, m->lou * sizeof(lr_MetalinkUrl));
    }

    m->urls[m->nou] = lr_malloc0(sizeof(struct _lr_MetalinkUrl));
    m->nou++;
    return m->urls[m->nou-1];
}

void
lr_free_metalinkhash(lr_MetalinkHash metalinkhash)
{
    if (!metalinkhash) return;
    lr_free(metalinkhash->type);
    lr_free(metalinkhash->value);
    lr_free(metalinkhash);
}

void
lr_free_metalinkurl(lr_MetalinkUrl metalinkurl)
{
    if (!metalinkurl) return;
    lr_free(metalinkurl->protocol);
    lr_free(metalinkurl->type);
    lr_free(metalinkurl->location);
    lr_free(metalinkurl->url);
    lr_free(metalinkurl);
}

lr_Metalink
lr_metalink_create()
{
    return lr_malloc0(sizeof(struct _lr_Metalink));
}

void
lr_metalink_free(lr_Metalink metalink)
{
    if (!metalink)
        return;
    lr_free(metalink->filename);
    for (int x = 0; x < metalink->noh; x++)
        lr_free_metalinkhash(metalink->hashes[x]);
    lr_free(metalink->hashes);
    for (int x = 0; x < metalink->nou; x++)
        lr_free_metalinkurl(metalink->urls[x]);
    lr_free(metalink->urls);
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
} lr_State;

typedef struct {
    lr_State from;  /*!< source state */
    char *ename;    /*!< element name */
    lr_State to;    /*!< target state */
    int docontent;  /*!< store the text of the element */
} lr_StatesSwitch;

/* Same states in the first column must be together */
static lr_StatesSwitch stateswitches[] = {
    { STATE_START,      "metalink",         STATE_METALINK,     0 },
    { STATE_METALINK,   "files",            STATE_FILES,        0 },
    { STATE_FILES,      "file",             STATE_FILE,         0 },
    { STATE_FILE,       "mm0:timestamp",    STATE_TIMESTAMP,    1 },
    { STATE_FILE,       "size",             STATE_SIZE,         1 },
    { STATE_FILE,       "verification",     STATE_VERIFICATION, 0 },
    { STATE_FILE,       "resources",        STATE_RESOURCES,    0 },
    { STATE_VERIFICATION, "hash",           STATE_HASH,         1 },
    { STATE_RESOURCES,  "url",              STATE_URL,          1 },
    { NUMSTATES }
};

typedef struct _ParserData {
    int ret;            /*!< status of parsing (return code) */
    int depth;
    int statedepth;
    lr_State state;     /*!< current state */

    int docontent;  /*!< tell if store text from the current element */
    char *content;  /*!< text content of element */
    int lcontent;   /*!< content lenght */
    int acontent;   /*!< availbable bytes in the content */

    XML_Parser *parser;                 /*!< parser */
    lr_StatesSwitch *swtab[NUMSTATES];  /*!< pointers to stateswitches table */
    lr_State sbtab[NUMSTATES];          /*!< stab[to_state] = from_state */

    lr_Metalink metalink;   /*!< metalink object */
} ParserData;

static inline const char *
lr_find_attr(const char *txt, const char **atts)
{
    for (; *atts; atts += 2)
        if (!strcmp(*atts, txt))
            return atts[1];
    return 0;
}

static void XMLCALL
lr_metalink_start_handler(void *pdata, const char *name, const char **atts)
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

    switch (pd->state) {
    case STATE_START:
    case STATE_METALINK:
    case STATE_FILES:
        break;

    case STATE_FILE: {
        const char *name = lr_find_attr("name", atts);
        if (!name) {
            pd->ret = LRE_ML_XML;
            break;
        }
        if (strcmp(name, "repomd.xml")) {
            pd->ret = LRE_ML_BAD;
            break;
        }
        pd->metalink->filename = lr_strdup(name);
        break;
    }
    case STATE_TIMESTAMP:
    case STATE_SIZE:
    case STATE_VERIFICATION:
        break;

    case STATE_HASH: {
        lr_MetalinkHash mh;
        const char *type = lr_find_attr("type", atts);
        if (!type) {
            pd->ret = LRE_ML_XML;
            break;
        }
        mh = lr_new_metalinkhash(pd->metalink);
        mh->type = lr_strdup(type);
        break;
    }

    case STATE_RESOURCES:
        break;

    case STATE_URL: {
        const char *attr;
        lr_MetalinkUrl url = lr_new_metalinkurl(pd->metalink);
        if ((attr = lr_find_attr("protocol", atts)))
            url->protocol = lr_strdup(attr);
        if ((attr = lr_find_attr("type", atts)))
            url->type = lr_strdup(attr);
        if ((attr = lr_find_attr("location", atts)))
            url->location = lr_strdup(attr);
        if ((attr = lr_find_attr("preference", atts)))
            url->preference = atol(attr);
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

    if (pd->ret != LRE_OK)
        return; /* There was an error -> do nothing */

    if (pd->depth != pd->statedepth) {
        /* Back from the unknown state */
        pd->depth--;
        return;
    }

    pd->depth--;
    pd->statedepth--;

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
        if (!pd->metalink->noh) {
            pd->ret = LRE_ML_XML;
            break;
        }
        pd->metalink->hashes[pd->metalink->noh-1]->value = lr_strdup(pd->content);
        break;

    case STATE_URL:
        if (!pd->metalink->nou) {
            pd->ret = LRE_ML_XML;
            break;
        }
        pd->metalink->urls[pd->metalink->nou-1]->url = lr_strdup(pd->content);
        break;

    default:
        break;
    };

    pd->state = pd->sbtab[pd->state];
    pd->docontent = 0;

    return;
}

int
lr_metalink_parse_file(lr_Metalink metalink, int fd)
{
    int ret = LRE_OK;
    XML_Parser parser;
    ParserData pd;
    lr_StatesSwitch *sw;

    assert(metalink);
    DEBUGASSERT(fd >= 0);

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
            ret = LRE_IO;
            break;
        }

        if (!XML_ParseBuffer(parser, len, len == 0)) {
            ret = LRE_ML_XML;
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
