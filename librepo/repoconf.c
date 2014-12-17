/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2014  Tomas Mlcoch
 * Copyright (C) 2014  Richard Hughes
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
#include <math.h>
#include <string.h>
#include <errno.h>
#include "librepo.h"
#include "repoconf.h"
#include "cleanup.h"

struct _LrYumRepoConfs {
    GSList *repos;
};

static LrYumRepoConf *
lr_yum_repoconf_init(void)
{
    LrYumRepoConf *repoconf = lr_malloc0(sizeof(*repoconf));

    // Set default values
    repoconf->bandwidth         = LR_YUMREPOCONF_BANDWIDTH_DEFAULT;
    repoconf->ip_resolve        = LR_YUMREPOCONF_IP_RESOLVE_DEFAULT;
    repoconf->metadata_expire   = LR_YUMREPOCONF_METADATA_EXPIRE_DEFAULT;
    repoconf->cost              = LR_YUMREPOCONF_COST_DEFAULT;
    repoconf->priority          = LR_YUMREPOCONF_PRIORITY_DEFAULT;

    return repoconf;
}

static void
lr_yum_repoconf_free(LrYumRepoConf *repoconf)
{
    if (!repoconf)
        return;

    g_free(repoconf->_source);

    g_free(repoconf->id);
    g_free(repoconf->name);
    g_strfreev(repoconf->baseurl);
    g_free(repoconf->mirrorlist);
    g_free(repoconf->metalink);

    g_free(repoconf->mediaid);
    g_strfreev(repoconf->gpgkey);
    g_strfreev(repoconf->gpgcakey);
    g_strfreev(repoconf->exclude);
    g_strfreev(repoconf->include);

    g_free(repoconf->proxy);
    g_free(repoconf->proxy_username);
    g_free(repoconf->proxy_password);
    g_free(repoconf->username);
    g_free(repoconf->password);

    g_free(repoconf->throttle);

    g_free(repoconf->sslcacert);
    g_free(repoconf->sslclientcert);
    g_free(repoconf->sslclientkey);

    g_strfreev(repoconf->deltarepobaseurl);

    g_free(repoconf);
}

LrYumRepoConf *
lr_yum_repoconf_copy(LrYumRepoConf *oth)
{
    LrYumRepoConf *repoconf = NULL;

    if (!oth)
        return NULL;

    repoconf = lr_yum_repoconf_init();

    repoconf->_source       = g_strdup(oth->_source);

    repoconf->id                = g_strdup(oth->id);
    repoconf->name              = g_strdup(oth->name);
    repoconf->enabled           = oth->enabled;
    repoconf->baseurl           = lr_strv_dup(oth->baseurl);
    repoconf->mirrorlist        = g_strdup(oth->mirrorlist);
    repoconf->metalink          = g_strdup(oth->metalink);

    repoconf->mediaid           = g_strdup(oth->mediaid);
    repoconf->gpgkey            = lr_strv_dup(oth->gpgkey);
    repoconf->gpgcakey          = lr_strv_dup(oth->gpgcakey);
    repoconf->exclude           = lr_strv_dup(oth->exclude);
    repoconf->include           = lr_strv_dup(oth->include);

    repoconf->fastestmirror     = oth->fastestmirror;
    repoconf->proxy             = g_strdup(oth->proxy);
    repoconf->proxy_username    = g_strdup(oth->proxy_username);
    repoconf->proxy_password    = g_strdup(oth->proxy_password);
    repoconf->username          = g_strdup(oth->username);
    repoconf->password          = g_strdup(oth->password);

    repoconf->gpgcheck          = oth->gpgcheck;
    repoconf->repo_gpgcheck     = oth->repo_gpgcheck;
    repoconf->enablegroups      = oth->enablegroups;

    repoconf->bandwidth         = oth->bandwidth;
    repoconf->throttle          = g_strdup(oth->throttle);
    repoconf->ip_resolve        = oth->ip_resolve;

    repoconf->metadata_expire   = oth->metadata_expire;
    repoconf->cost              = oth->cost;
    repoconf->priority          = oth->priority;

    repoconf->sslcacert         = g_strdup(oth->sslcacert);
    repoconf->sslverify         = oth->sslverify;
    repoconf->sslclientcert     = g_strdup(oth->sslclientcert);
    repoconf->sslclientkey      = g_strdup(oth->sslclientkey);

    repoconf->deltarepobaseurl  = lr_strv_dup(oth->deltarepobaseurl);

    return repoconf;
}

LrYumRepoConfs *
lr_yum_repoconfs_init(void)
{
    LrYumRepoConfs *repos = lr_malloc0(sizeof(*repos));
    return repos;
}

void
lr_yum_repoconfs_free(LrYumRepoConfs *repos)
{
    g_slist_free_full(repos->repos, (GDestroyNotify) lr_yum_repoconf_free);
    g_free(repos);
}

GSList *
lr_yum_repoconfs_get_list(LrYumRepoConfs *repos, G_GNUC_UNUSED GError **err)
{
    return repos->repos;
}

/* This function is taken from libhif
 * Original author: Richard Hughes <richard at hughsie dot com>
 */
static GKeyFile *
lr_load_multiline_key_file(const char *filename,
                           GError **err)
{
    GKeyFile *file = NULL;
    gboolean ret;
    gsize len;
    guint i;
    _cleanup_free_ gchar *data = NULL;
    _cleanup_string_free_ GString *string = NULL;
    _cleanup_strv_free_ gchar **lines = NULL;

    // load file
    if (!g_file_get_contents (filename, &data, &len, err))
        return NULL;

    // split into lines
    string = g_string_new ("");
    lines = g_strsplit (data, "\n", -1);
    for (i = 0; lines[i] != NULL; i++) {

        // convert tabs to spaces
        g_strdelimit (lines[i], "\t", ' ');

        // if a line starts with whitespace, then append it on
        // the previous line
        if (lines[i][0] == ' ' && string->len > 0) {

            // remove old newline from previous line
            g_string_set_size (string, string->len - 1);

            // whitespace strip this new line
            g_strchug (lines[i]);

            // only add a ';' if we have anything after the '='
            if (string->str[string->len - 1] == '=') {
                g_string_append_printf (string, "%s\n", lines[i]);
            } else {
                g_string_append_printf (string, ";%s\n", lines[i]);
            }
        } else {
            g_string_append_printf (string, "%s\n", lines[i]);
        }
    }

    // remove final newline
    if (string->len > 0)
        g_string_set_size (string, string->len - 1);

    // load modified lines
    file = g_key_file_new ();
    ret = g_key_file_load_from_data (file,
                                     string->str,
                                     -1,
                                     G_KEY_FILE_KEEP_COMMENTS,
                                     err);
    if (!ret) {
        g_key_file_free (file);
        return NULL;
    }
    return file;
}

static gboolean
lr_key_file_get_boolean(GKeyFile *keyfile,
                        const gchar *groupname,
                        const gchar *key,
                        gboolean default_value,
                        GError **err)
{
    _cleanup_free_ gchar *string = NULL;
    _cleanup_free_ gchar *string_lower = NULL;
    if (!g_key_file_has_key (keyfile, groupname, key, NULL))
        return default_value;
    string = g_key_file_get_string(keyfile, groupname, key, err);
    string_lower = g_ascii_strdown (string, -1);
    if (!g_strcmp0(string_lower, "1") ||
        !g_strcmp0(string_lower, "yes") ||
        !g_strcmp0(string_lower, "true"))
        return TRUE;
    return FALSE;
}

static gint
lr_key_file_get_integer(GKeyFile *keyfile,
                        const gchar *groupname,
                        const gchar *key,
                        gint default_value,
                        G_GNUC_UNUSED GError **err)
{
    if (g_key_file_has_key (keyfile, groupname, key, NULL))
        return g_key_file_get_integer(keyfile, groupname, key, NULL);
    return default_value;
}

static gchar **
lr_key_file_get_string_list(GKeyFile *keyfile,
                            const gchar *groupname,
                            const gchar *key,
                            GError **err)
{
    gchar **list = NULL;
    _cleanup_free_ gchar *string = NULL;
    string = g_key_file_get_string(keyfile, groupname, key, err);
    if (!string)
        return list;
    list = g_strsplit_set(string, " ,;", 0);
    for (gint i=0; list && list[i]; i++)
        g_strstrip(list[i]);
    return list;
}

static LrIpResolveType
lr_key_file_get_ip_resolve(GKeyFile *keyfile,
                            const gchar *groupname,
                            const gchar *key,
                            LrIpResolveType default_value,
                            GError **err)
{
    _cleanup_free_ gchar *string = NULL;
    _cleanup_free_ gchar *string_lower = NULL;
    if (!g_key_file_has_key (keyfile, groupname, key, NULL))
        return default_value;
    string = g_key_file_get_string(keyfile, groupname, key, err);
    string_lower = g_ascii_strdown(string, -1);
    if (!g_strcmp0(string_lower, "ipv4"))
        return LR_IPRESOLVE_V4;
    else if (!g_strcmp0(string_lower, "ipv6"))
        return LR_IPRESOLVE_V6;
    else if (!g_strcmp0(string_lower, "whatever"))
        return LR_IPRESOLVE_WHATEVER;
    else
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Unknown ip_resolve value '%s'", string);
    return default_value;
}

static gboolean
lr_convert_interval_to_seconds(const char *str,
                               gint64 *out,
                               GError **err)
{
    gdouble value = 0.0;
    gdouble mult = 1.0;
    gchar *endptr = NULL;

    *out = 0;

    // Initial sanity checking
    if (!str) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_BADFUNCARG,
                    "No time interval value specified");
        return FALSE;
    }

    // Initial conversion
    value = g_ascii_strtod(str, &endptr);
    if (value == HUGE_VAL && errno == ERANGE) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Too big time interval value '%s'", str);
        return FALSE;
    }

    // String doesn't start with numbers
    if (endptr == str) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Could't convert '%s' to seconds", str);
        return FALSE;
    }

    // Process multiplier (if supplied)
    if (endptr) {
        if (strlen(endptr) != 1) {
            g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                        "Unknown time interval unit '%s'", endptr);
            return FALSE;
        }

        gchar unit = g_ascii_tolower(*endptr);
        if (unit == 's') {
            mult = 1.0;
        } else if (unit == 'm') {
            mult = 60.0;
        } else if (unit == 'h') {
            mult = 60.0 * 60.0;
        } else if (unit == 'd') {
            mult = 60.0 * 60.0 * 24.0;
        } else {
            g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                        "Unknown time interval unit '%s'", endptr);
            return FALSE;
        }
    }

    // Convert result to seconds
    value = value * mult;

    // Return result as integer
    *out = (gint64) value;
    return TRUE;
}

static gint64
lr_key_file_get_metadata_expire(GKeyFile *keyfile,
                                const gchar *groupname,
                                const gchar *key,
                                gint64 default_value,
                                GError **err)
{
    gint64 res = -1;
    _cleanup_free_ gchar *string = NULL;
    if (!g_key_file_has_key (keyfile, groupname, key, NULL))
        return default_value;
    string = g_key_file_get_string(keyfile, groupname, key, err);
    if (!lr_convert_interval_to_seconds(string, &(res), err))
        return -1;
    return res;
}

static gboolean
lr_convert_bandwidth_to_bytes(const char *str,
                              guint64 *out,
                              GError **err)
{
    gdouble dbytes = 0.0;
    gdouble mult = 1.0;
    gchar *endptr = NULL;

    *out = 0;

    // Initial sanity checking
    if (!str) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_BADFUNCARG,
                    "No bandwidth value specified");
        return FALSE;
    }

    // Initial conversion
    dbytes = g_ascii_strtod(str, &endptr);
    if (dbytes == HUGE_VAL && errno == ERANGE) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Too big bandwidth value '%s'", str);
        return FALSE;
    }

    // String doesn't start with numbers
    if (endptr == str) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Could't convert '%s' to number", str);
        return FALSE;
    }

    // Process multiplier (if supplied)
    if (endptr) {
        if (strlen(endptr) != 1) {
            g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                        "Unknown unit '%s'", endptr);
            return FALSE;
        }

        gchar unit = g_ascii_tolower(*endptr);
        if (unit == 'k') {
            mult = 1024.0;
        } else if (unit == 'm') {
            mult = 1024.0 * 1024.0;
        } else if (unit == 'g') {
            mult = 1024.0 * 1024.0 * 1024.0;
        } else {
            g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                        "Unknown unit '%s'", endptr);
            return FALSE;
        }
    }

    // Convert result to bytes
    dbytes = dbytes * mult;
    if (dbytes < 0.0) {
        g_set_error(err, LR_REPOCONF_ERROR, LRE_VALUE,
                    "Bytes value may not be negative '%s'", str);
        return FALSE;
    }

    // Return result as integer
    *out = (guint64) dbytes;
    return TRUE;
}

guint64
lr_key_file_get_bandwidth(GKeyFile *keyfile,
                          const gchar *groupname,
                          const gchar *key,
                          guint64 default_value,
                          GError **err)
{
    guint64 res = 0;
    _cleanup_free_ gchar *string = NULL;
    if (!g_key_file_has_key (keyfile, groupname, key, NULL))
        return default_value;
    string = g_key_file_get_string(keyfile, groupname, key, err);
    if (!lr_convert_bandwidth_to_bytes(string, &(res), err))
        return 0;
    return res;
}

static gboolean
lr_yum_repoconf_parse_id(LrYumRepoConf **yumconf,
                         const gchar *id,
                         const gchar *filename,
                         GKeyFile *keyfile,
                         G_GNUC_UNUSED GError **err)
{
    LrYumRepoConf *conf = lr_yum_repoconf_init();
    _cleanup_error_free_ GError *tmp_err = NULL;

    // _source
    conf->_source = g_strdup(filename);

    // id
    conf->id = g_strdup(id);

    // name
    conf->name = g_key_file_get_string(keyfile, id, "name", NULL);

    // enabled
    conf->enabled = lr_key_file_get_boolean (keyfile, id, "enabled", TRUE, NULL);

    // baseurl
    conf->baseurl = lr_key_file_get_string_list(keyfile, id, "baseurl", NULL);

    // mirrorlist
    conf->mirrorlist = g_key_file_get_string(keyfile, id, "mirrorlist", NULL);

    // metalink
    conf->metalink = g_key_file_get_string(keyfile, id, "metalink", NULL);

    // mediaid
    conf->mediaid = g_key_file_get_string(keyfile, id, "mediaid", NULL);

    // gpgkey
    conf->gpgkey = lr_key_file_get_string_list(keyfile, id, "gpgkey", NULL);

    // gpgcakey
    conf->gpgcakey = lr_key_file_get_string_list(keyfile, id, "gpgcakey", NULL);

    // exclude
    conf->exclude = lr_key_file_get_string_list(keyfile, id, "exclude", NULL);

    // include
    conf->include = lr_key_file_get_string_list(keyfile, id, "include", NULL);

    // fastestmirror
    conf->fastestmirror = lr_key_file_get_boolean (keyfile, id, "fastestmirror", FALSE, NULL);

    // proxy
    conf->proxy = g_key_file_get_string(keyfile, id, "proxy", NULL);

    // proxy_username
    conf->proxy_username = g_key_file_get_string(keyfile, id, "proxy_username", NULL);

    // proxy_password
    conf->proxy_password = g_key_file_get_string(keyfile, id, "proxy_password", NULL);

    // username
    conf->username = g_key_file_get_string(keyfile, id, "username", NULL);

    // password
    conf->password = g_key_file_get_string(keyfile, id, "password", NULL);

    // gpgcheck
    conf->gpgcheck = lr_key_file_get_boolean (keyfile, id, "gpgcheck", FALSE, NULL);

    // repo_gpgcheck
    conf->repo_gpgcheck = lr_key_file_get_boolean (keyfile, id, "repo_gpgcheck", FALSE, NULL);

    // enablegroups
    conf->enablegroups = lr_key_file_get_boolean (keyfile, id, "enablegroups", TRUE, NULL);

    // bandwidth
    conf->bandwidth = lr_key_file_get_bandwidth(keyfile, id, "bandwidth", LR_YUMREPOCONF_BANDWIDTH_DEFAULT, &tmp_err);
    if (tmp_err)
        goto err;

    // throttle
    conf->throttle = g_key_file_get_string(keyfile, id, "throttle", NULL);

    // ip_resolve
    conf->ip_resolve = lr_key_file_get_ip_resolve(keyfile, id, "ip_resolve", LR_YUMREPOCONF_IP_RESOLVE_DEFAULT, NULL);

    // metadata_expire
    conf->metadata_expire = lr_key_file_get_metadata_expire(keyfile, id, "metadata_expire", LR_YUMREPOCONF_METADATA_EXPIRE_DEFAULT, NULL);

    // cost
    conf->cost = lr_key_file_get_integer(keyfile, id, "cost", LR_YUMREPOCONF_COST_DEFAULT, NULL);

    // priority
    conf->priority = lr_key_file_get_integer(keyfile, id, "priority", LR_YUMREPOCONF_PRIORITY_DEFAULT, NULL);

    // sslcacert
    conf->sslcacert = g_key_file_get_string(keyfile, id, "sslcacert", NULL);

    // sslverify
    conf->sslverify = lr_key_file_get_boolean (keyfile, id, "sslverify", TRUE, NULL);

    // sslclientcert
    conf->sslclientcert = g_key_file_get_string(keyfile, id, "sslclientcert", NULL);

    // sslclientkey
    conf->sslclientkey = g_key_file_get_string(keyfile, id, "sslclientkey", NULL);

    // deltarepobaseurl
    conf->deltarepobaseurl = lr_key_file_get_string_list(keyfile, id, "deltarepobaseurl", NULL);

    *yumconf = conf;
    return TRUE;

err:
    lr_yum_repoconf_free(conf);
    *yumconf = NULL;
    return FALSE;
}

gboolean
lr_yum_repoconfs_parse(LrYumRepoConfs *repos,
                       const char *filename,
                       GError **err)
{
    gboolean ret = TRUE;
    _cleanup_strv_free_ gchar **groups = NULL;
    _cleanup_keyfile_free_ GKeyFile *keyfile = NULL;

    keyfile = lr_load_multiline_key_file(filename, err);
    if (!keyfile)
        return FALSE;

    groups = g_key_file_get_groups (keyfile, NULL);
    for (guint i = 0; groups[i]; i++) {
        LrYumRepoConf *repoconf = NULL;
        ret = lr_yum_repoconf_parse_id(&repoconf,
                                       groups[i],
                                       filename,
                                       keyfile,
                                       err);
        if (!ret)
            return FALSE;
        repos->repos = g_slist_append(repos->repos, repoconf);
    }

    return ret;
}

gboolean
lr_yum_repoconfs_load_dir(LrYumRepoConfs *repos,
                          const char *path,
                          GError **err)
{
    const gchar *file;
    _cleanup_dir_close_ GDir *dir = NULL;

    // Open dir
    dir = g_dir_open(path, 0, err);
    if (!dir)
        return FALSE;

    // Find all the .repo files
    while ((file = g_dir_read_name(dir))) {
        _cleanup_free_ gchar *path_tmp = NULL;
        if (!g_str_has_suffix(file, ".repo"))
            continue;
        path_tmp = g_build_filename(path, file, NULL);
        if (!lr_yum_repoconfs_parse(repos, path_tmp, err))
            return FALSE;
    }

    return TRUE;
}
