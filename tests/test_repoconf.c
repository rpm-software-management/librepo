#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "testsys.h"
#include "fixtures.h"
#include "test_repomd.h"
#include "librepo/rcodes.h"
#include "librepo/types.h"
#include "librepo/repoconf.h"
#include "librepo/util.h"

#include "librepo/cleanup.h"



static void
repoconf_assert_true(LrYumRepoConf *repoconf,
                     LrYumRepoConfOption option)
{
    ck_assert(1);
    long val = 0L;
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_msg(val != 0, "Not a True value (Option %d)", option);
}

#define conf_assert_true(option) \
            repoconf_assert_true(conf, (option))

static void
repoconf_assert_false(LrYumRepoConf *repoconf,
                      LrYumRepoConfOption option)
{
    ck_assert(1);
    long val = 1L;
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_msg(!val, "Not a 0 (False) value (Option %d)", option);
}

#define conf_assert_false(option) \
            repoconf_assert_false(conf, (option))

static void
repoconf_assert_na(LrYumRepoConf *repoconf,
                   LrYumRepoConfOption option)
{
    ck_assert(1);
    char array[2048];
    /* ^^^XXX the lr_yum_repoconf_getinfo always expects that you have
     * passed an argument of an appropriate size and even when an error
     * occurs, it can write a value (e.g. default value) into the
     * destination. Because we don't care about the value and the size
     * are different for different types (char*, gint64, long, ...)
     * we pass in an array with (hopefully) enough space to accommodate
     * all reasonable return values. */
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, array);
    ck_assert(!ret);
    ck_assert(tmp_err != NULL);
    ck_assert(tmp_err->code == LRE_NOTSET);
}

#define conf_assert_na(option) \
            repoconf_assert_na(conf, (option))

static void
repoconf_assert_str_eq(LrYumRepoConf *repoconf,
                       LrYumRepoConfOption option,
                       gchar *expected)
{
    ck_assert(1);
    _cleanup_free_ gchar *str = NULL;
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &str);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert(g_strcmp0(str, expected) == 0);
}

#define conf_assert_str_eq(option, expected) \
            repoconf_assert_str_eq(conf, (option), (expected))

static void
repoconf_assert_uint64_eq(LrYumRepoConf *repoconf,
                          LrYumRepoConfOption option,
                          guint64 expected)
{
    ck_assert(1);
    guint64 val = (expected - 1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert(val == expected);
}

#define conf_assert_uint64_eq(option, expected) \
            repoconf_assert_uint64_eq(conf, (option), (expected))

static void
repoconf_assert_int_eq(LrYumRepoConf *repoconf,
                       LrYumRepoConfOption option,
                       gint expected)
{
    ck_assert(1);
    gint val = (expected - 1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_int_eq(val, expected);
}

#define conf_assert_int_eq(option, expected) \
            repoconf_assert_int_eq(conf, (option), (expected))

static void
repoconf_assert_int64_eq(LrYumRepoConf *repoconf,
                         LrYumRepoConfOption option,
                         gint64 expected)
{
    ck_assert(1);
    gint64 val = (expected - 1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_int_eq(val, expected);
}

#define conf_assert_int64_eq(option, expected) \
            repoconf_assert_int64_eq(conf, (option), (expected))

static void
repoconf_assert_lripresolvetype_eq(LrYumRepoConf *repoconf,
                                   LrYumRepoConfOption option,
                                   LrIpResolveType expected)
{
    ck_assert(1);
    LrIpResolveType val = (expected - 1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_msg((val == expected), "IpResolve assert failed %d != %d", val, expected);
}

#define conf_assert_lripresolvetype_eq(option, expected) \
            repoconf_assert_lripresolvetype_eq(conf, (option), (expected))


static void
repoconf_assert_strv_eq(LrYumRepoConf *repoconf,
                           LrYumRepoConfOption option,
                           ...)
{
    ck_assert(1);
    va_list args;
    _cleanup_error_free_ GError *tmp_err = NULL;
    _cleanup_strv_free_ char **strv = NULL;

    gboolean ret = lr_yum_repoconf_getinfo(repoconf, &tmp_err, option, &strv);
    ck_assert(!tmp_err);
    ck_assert(ret);
    ck_assert_msg(strv != NULL, "NULL isn't strv");

    va_start (args, option);
    gchar **strv_p = strv;
    for (; *strv_p; strv_p++) {
        gchar *s = va_arg (args, gchar*);
        ck_assert_msg(s != NULL, "Lengths of lists are not the same");
        ck_assert_str_eq(*strv_p, s);
    }

    // strv list already ended, check if we really expected the end
    ck_assert_msg(va_arg(args, gchar*) == NULL,
                  "Lengths of lists are not the same");

    va_end (args);
}

#define conf_assert_strv_eq(option, ...) \
            repoconf_assert_strv_eq(conf, (option), __VA_ARGS__)

static void
repoconf_assert_set_boolean(LrYumRepoConf *repoconf,
                            LrYumRepoConfOption option,
                            gboolean val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_boolean(option, val) \
            repoconf_assert_set_boolean(conf, (option), (val))

static void
repoconf_assert_set_str(LrYumRepoConf *repoconf,
                        LrYumRepoConfOption option,
                        gchar *val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_str(option, val) \
            repoconf_assert_set_str(conf, (option), (val))

static void
repoconf_assert_set_strv(LrYumRepoConf *repoconf,
                         LrYumRepoConfOption option,
                         ...)
{
    ck_assert(1);
    va_list args;
    _cleanup_error_free_ GError *tmp_err = NULL;
    _cleanup_ptrarray_unref_ GPtrArray *array = g_ptr_array_new();

    // Build strv from VA args
    va_start (args, option);
    for (gchar *s=va_arg(args, gchar*); s; s=va_arg(args, gchar*))
        g_ptr_array_add(array, s);
    g_ptr_array_add(array, NULL);
    va_end (args);

    // Set the option
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, array->pdata);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_strv(option, ...) \
            repoconf_assert_set_strv(conf, (option), __VA_ARGS__)

static void
repoconf_assert_set_int(LrYumRepoConf *repoconf,
                        LrYumRepoConfOption option,
                        gint val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_int(option, val) \
            repoconf_assert_set_int(conf, (option), (val))

static void
repoconf_assert_set_int64(LrYumRepoConf *repoconf,
                          LrYumRepoConfOption option,
                          gint64 val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_int64(option, val) \
            repoconf_assert_set_int64(conf, (option), (val))

static void
repoconf_assert_set_uint64(LrYumRepoConf *repoconf,
                           LrYumRepoConfOption option,
                           guint64 val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_uint64(option, val) \
            repoconf_assert_set_uint64(conf, (option), (val))

static void
repoconf_assert_set_lripresolvetype(LrYumRepoConf *repoconf,
                                    LrYumRepoConfOption option,
                                    LrIpResolveType val)
{
    ck_assert(1);
    _cleanup_error_free_ GError *tmp_err = NULL;
    gboolean ret = lr_yum_repoconf_setopt(repoconf, &tmp_err, option, val);
    ck_assert(!tmp_err);
    ck_assert(ret);
}

#define conf_assert_set_lripresolvetype(option, val) \
            repoconf_assert_set_lripresolvetype(conf, (option), (val))


START_TEST(test_parse_repoconf_minimal)
{
    gboolean ret;
    LrYumRepoConf *conf = NULL;
    LrYumRepoConfs *confs = NULL;
    GSList *list = NULL;
    _cleanup_free_ gchar *path = NULL;
    _cleanup_error_free_ GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, "repo-minimal.repo", NULL);

    confs = lr_yum_repoconfs_init();
    fail_if(!confs);

    ret = lr_yum_repoconfs_parse(confs, path, &tmp_err);
    fail_if(!ret);

    list = lr_yum_repoconfs_get_list(confs, &tmp_err);
    fail_if(!list);
    fail_if(g_slist_length(list) != 2);

    // Test content of first repo config

    conf = g_slist_nth_data(list, 0);
    fail_if(!conf);

    conf_assert_str_eq(LR_YRC_ID, "minimal-repo-1");
    conf_assert_str_eq(LR_YRC_NAME, "Minimal repo 1 - $basearch");
    conf_assert_na(LR_YRC_ENABLED);
    conf_assert_strv_eq(LR_YRC_BASEURL, "http://m1.com/linux/$basearch", NULL);
    conf_assert_na(LR_YRC_MIRRORLIST);
    conf_assert_na(LR_YRC_METALINK);

    conf_assert_na(LR_YRC_MEDIAID);
    conf_assert_na(LR_YRC_GPGKEY);
    conf_assert_na(LR_YRC_GPGCAKEY);
    conf_assert_na(LR_YRC_EXCLUDE);
    conf_assert_na(LR_YRC_INCLUDE);

    conf_assert_na(LR_YRC_FASTESTMIRROR);
    conf_assert_na(LR_YRC_PROXY);
    conf_assert_na(LR_YRC_PROXY_USERNAME);
    conf_assert_na(LR_YRC_PROXY_PASSWORD);
    conf_assert_na(LR_YRC_USERNAME);
    conf_assert_na(LR_YRC_PASSWORD);

    conf_assert_na(LR_YRC_GPGCHECK);
    conf_assert_na(LR_YRC_REPO_GPGCHECK);
    conf_assert_na(LR_YRC_ENABLEGROUPS);

    conf_assert_na(LR_YRC_BANDWIDTH);
    conf_assert_na(LR_YRC_THROTTLE);
    conf_assert_na(LR_YRC_IP_RESOLVE);

    conf_assert_na(LR_YRC_METADATA_EXPIRE);
    conf_assert_na(LR_YRC_COST);
    conf_assert_na(LR_YRC_PRIORITY);

    conf_assert_na(LR_YRC_SSLCACERT);
    conf_assert_na(LR_YRC_SSLVERIFY);
    conf_assert_na(LR_YRC_SSLCLIENTCERT);
    conf_assert_na(LR_YRC_SSLCLIENTKEY);

    conf_assert_na(LR_YRC_DELTAREPOBASEURL);

    conf_assert_na(LR_YRC_FAILOVERMETHOD);
    conf_assert_na(LR_YRC_SKIP_IF_UNAVAILABLE);

    // Test content of second repo config

    conf = g_slist_nth_data(list, 1);
    fail_if(!conf);

    conf_assert_str_eq(LR_YRC_ID, "minimal-repo-2");
    conf_assert_str_eq(LR_YRC_NAME, "Minimal repo 2 - $basearch");
    conf_assert_na(LR_YRC_ENABLED);
    conf_assert_strv_eq(LR_YRC_BASEURL, "http://m2.com/linux/$basearch", NULL);
    conf_assert_na(LR_YRC_MIRRORLIST);
    conf_assert_na(LR_YRC_METALINK);

    conf_assert_na(LR_YRC_MEDIAID);
    conf_assert_na(LR_YRC_GPGKEY);
    conf_assert_na(LR_YRC_GPGCAKEY);
    conf_assert_na(LR_YRC_EXCLUDE);
    conf_assert_na(LR_YRC_INCLUDE);

    conf_assert_na(LR_YRC_FASTESTMIRROR);
    conf_assert_na(LR_YRC_PROXY);
    conf_assert_na(LR_YRC_PROXY_USERNAME);
    conf_assert_na(LR_YRC_PROXY_PASSWORD);
    conf_assert_na(LR_YRC_USERNAME);
    conf_assert_na(LR_YRC_PASSWORD);

    conf_assert_na(LR_YRC_GPGCHECK);
    conf_assert_na(LR_YRC_REPO_GPGCHECK);
    conf_assert_na(LR_YRC_ENABLEGROUPS);

    conf_assert_na(LR_YRC_BANDWIDTH);
    conf_assert_na(LR_YRC_THROTTLE);
    conf_assert_na(LR_YRC_IP_RESOLVE);

    conf_assert_na(LR_YRC_METADATA_EXPIRE);
    conf_assert_na(LR_YRC_COST);
    conf_assert_na(LR_YRC_PRIORITY);

    conf_assert_na(LR_YRC_SSLCACERT);
    conf_assert_na(LR_YRC_SSLVERIFY);
    conf_assert_na(LR_YRC_SSLCLIENTCERT);
    conf_assert_na(LR_YRC_SSLCLIENTKEY);

    conf_assert_na(LR_YRC_FAILOVERMETHOD);

    conf_assert_na(LR_YRC_SKIP_IF_UNAVAILABLE);
    conf_assert_na(LR_YRC_DELTAREPOBASEURL);

    lr_yum_repoconfs_free(confs);
}
END_TEST

START_TEST(test_parse_repoconf_big)
{
    gboolean ret;
    LrYumRepoConf *conf = NULL;
    LrYumRepoConfs *confs = NULL;
    GSList *list = NULL;
    _cleanup_free_ gchar *path = NULL;
    _cleanup_error_free_ GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, "repo-big.repo", NULL);

    confs = lr_yum_repoconfs_init();
    fail_if(!confs);

    ret = lr_yum_repoconfs_parse(confs, path, &tmp_err);
    fail_if(!ret);

    list = lr_yum_repoconfs_get_list(confs, &tmp_err);
    fail_if(!list);
    fail_if(g_slist_length(list) != 1);

    conf = g_slist_nth_data(list, 0);
    fail_if(!conf);

    conf_assert_str_eq(LR_YRC_ID, "big-repo");
    conf_assert_str_eq(LR_YRC_NAME, "Maxi repo - $basearch");
    conf_assert_true(LR_YRC_ENABLED);
    conf_assert_strv_eq(LR_YRC_BASEURL,
                      "http://foo1.org/pub/linux/$releasever/$basearch/os/",
                      "ftp://ftp.foo2/pub/linux/$releasever/$basearch/os/",
                      "https://foo3.org/pub/linux/",
                      NULL);
    conf_assert_str_eq(LR_YRC_MIRRORLIST,"http://foo1.org/mirrorlist");
    conf_assert_str_eq(LR_YRC_METALINK, "https://foo1.org/metalink?repo=linux-$releasever&arch=$basearch");

    conf_assert_str_eq(LR_YRC_MEDIAID,     "0");
    conf_assert_strv_eq(LR_YRC_GPGKEY,
                      "https://foo1.org/linux/foo_signing_key.pub",
                      "https://foo2.org/linux/foo_signing_key.pub",
                      NULL);
    conf_assert_strv_eq(LR_YRC_GPGCAKEY, "https://foo1.org/linux/ca_key.pub", NULL);
    conf_assert_strv_eq(LR_YRC_EXCLUDE, "package_1", "package_2", NULL);
    conf_assert_strv_eq(LR_YRC_INCLUDE, "package_a", "package_b", NULL);

    conf_assert_true(LR_YRC_FASTESTMIRROR);
    conf_assert_str_eq(LR_YRC_PROXY, "socks5://127.0.0.1:5000");
    conf_assert_str_eq(LR_YRC_PROXY_USERNAME, "proxyuser");
    conf_assert_str_eq(LR_YRC_PROXY_PASSWORD, "proxypass");
    conf_assert_str_eq(LR_YRC_USERNAME, "user");
    conf_assert_str_eq(LR_YRC_PASSWORD, "pass");

    conf_assert_true(LR_YRC_GPGCHECK);
    conf_assert_true(LR_YRC_REPO_GPGCHECK);
    conf_assert_true(LR_YRC_ENABLEGROUPS);

    conf_assert_uint64_eq(LR_YRC_BANDWIDTH, G_GUINT64_CONSTANT(1048576));  // 1024*1024
    conf_assert_str_eq(LR_YRC_THROTTLE, "50%");
    conf_assert_lripresolvetype_eq(LR_YRC_IP_RESOLVE, LR_IPRESOLVE_V6);

    conf_assert_int64_eq(LR_YRC_METADATA_EXPIRE, G_GINT64_CONSTANT(432000)); // 60*60*24*5
    conf_assert_int_eq(LR_YRC_COST, 500);
    conf_assert_int_eq(LR_YRC_PRIORITY, 10);

    conf_assert_str_eq(LR_YRC_SSLCACERT, "file:///etc/ssl.cert");
    conf_assert_true(LR_YRC_SSLVERIFY);
    conf_assert_str_eq(LR_YRC_SSLCLIENTCERT, "file:///etc/client.cert");
    conf_assert_str_eq(LR_YRC_SSLCLIENTKEY, "file:///etc/client.key");

    conf_assert_strv_eq(LR_YRC_DELTAREPOBASEURL,
                      "http://deltarepomirror.org/",
                      "http://deltarepomirror2.org",
                      NULL);

    conf_assert_str_eq(LR_YRC_FAILOVERMETHOD, "priority");
    conf_assert_true(LR_YRC_SKIP_IF_UNAVAILABLE);

    lr_yum_repoconfs_free(confs);
}
END_TEST

START_TEST(test_write_repoconf)
{
    _cleanup_file_close_ int rc = -1;
    gboolean ret;
    LrYumRepoConfs *confs;
    LrYumRepoConf * conf;
    char tmpfn[] = "/tmp/librepo_repoconf_test_XXXXXX";
    const char *ids[] = {"test_id", NULL};
    GSList *repos = NULL;
    _cleanup_error_free_ GError *tmp_err = NULL;

    // Create a temporary file
    rc = mkstemp(tmpfn);
    fail_if(rc == -1);

    // Create reconfs with one repoconf with one id (one section)
    confs = lr_yum_repoconfs_init();
    ret = lr_yum_repoconfs_add_empty_conf(confs, tmpfn, ids, &tmp_err);
    ck_assert(ret);
    ck_assert(!tmp_err);

    // Check if the repoconf with the section was created
    repos = lr_yum_repoconfs_get_list(confs, &tmp_err);
    ck_assert(!tmp_err);
    ck_assert_msg(repos != NULL, "No repo has been created");
    conf = g_slist_nth_data(repos, 0);

    // Ty to set and get options
    conf_assert_str_eq(LR_YRC_ID, "test_id");

    conf_assert_na(LR_YRC_NAME);
    conf_assert_set_str(LR_YRC_NAME, "test_name");
    conf_assert_str_eq(LR_YRC_NAME, "test_name");

    conf_assert_na(LR_YRC_ENABLED);
    conf_assert_set_boolean(LR_YRC_ENABLED, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_ENABLED);
    ck_assert(1);

    conf_assert_na(LR_YRC_BASEURL);
    conf_assert_set_strv(LR_YRC_BASEURL, "test_baseurl1", "test_baseurl2", NULL);
    conf_assert_strv_eq(LR_YRC_BASEURL, "test_baseurl1", "test_baseurl2", NULL);

    conf_assert_na(LR_YRC_MIRRORLIST);
    conf_assert_set_str(LR_YRC_MIRRORLIST, "test_mirrorlist");
    conf_assert_str_eq(LR_YRC_MIRRORLIST, "test_mirrorlist");

    conf_assert_na(LR_YRC_METALINK);
    conf_assert_set_str(LR_YRC_METALINK, "test_metalink");
    conf_assert_str_eq(LR_YRC_METALINK, "test_metalink");


    conf_assert_na(LR_YRC_MEDIAID);
    conf_assert_set_str(LR_YRC_MEDIAID, "test_mediaid");
    conf_assert_str_eq(LR_YRC_MEDIAID, "test_mediaid");

    conf_assert_na(LR_YRC_GPGKEY);
    conf_assert_set_strv(LR_YRC_GPGKEY, "test_gpgkey", NULL);
    conf_assert_strv_eq(LR_YRC_GPGKEY, "test_gpgkey", NULL);

    conf_assert_na(LR_YRC_GPGCAKEY);
    conf_assert_set_strv(LR_YRC_GPGCAKEY, "test_gpgcakey", NULL);
    conf_assert_strv_eq(LR_YRC_GPGCAKEY, "test_gpgcakey", NULL);

    conf_assert_na(LR_YRC_EXCLUDE);
    conf_assert_set_strv(LR_YRC_EXCLUDE, "test_exclude", NULL);
    conf_assert_strv_eq(LR_YRC_EXCLUDE, "test_exclude", NULL);

    conf_assert_na(LR_YRC_INCLUDE);
    conf_assert_set_strv(LR_YRC_INCLUDE, "test_include", NULL);
    conf_assert_strv_eq(LR_YRC_INCLUDE, "test_include", NULL);


    conf_assert_na(LR_YRC_FASTESTMIRROR);
    conf_assert_set_boolean(LR_YRC_FASTESTMIRROR, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_FASTESTMIRROR);
    ck_assert(1);

    conf_assert_na(LR_YRC_PROXY);
    conf_assert_set_str(LR_YRC_PROXY, "test_proxy");
    conf_assert_str_eq(LR_YRC_PROXY, "test_proxy");

    conf_assert_na(LR_YRC_PROXY_USERNAME);
    conf_assert_set_str(LR_YRC_PROXY_USERNAME, "test_proxy_username");
    conf_assert_str_eq(LR_YRC_PROXY_USERNAME, "test_proxy_username");

    conf_assert_na(LR_YRC_PROXY_PASSWORD);
    conf_assert_set_str(LR_YRC_PROXY_PASSWORD, "test_proxy_password");
    conf_assert_str_eq(LR_YRC_PROXY_PASSWORD, "test_proxy_password");

    conf_assert_na(LR_YRC_USERNAME);
    conf_assert_set_str(LR_YRC_USERNAME, "test_username");
    conf_assert_str_eq(LR_YRC_USERNAME, "test_username");

    conf_assert_na(LR_YRC_PASSWORD);
    conf_assert_set_str(LR_YRC_PASSWORD, "test_password");
    conf_assert_str_eq(LR_YRC_PASSWORD, "test_password");


    conf_assert_na(LR_YRC_GPGCHECK);
    conf_assert_set_boolean(LR_YRC_GPGCHECK, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_GPGCHECK);
    ck_assert(1);

    conf_assert_na(LR_YRC_REPO_GPGCHECK);
    conf_assert_set_boolean(LR_YRC_REPO_GPGCHECK, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_REPO_GPGCHECK);
    ck_assert(1);

    conf_assert_na(LR_YRC_ENABLEGROUPS);
    conf_assert_set_boolean(LR_YRC_ENABLEGROUPS, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_ENABLEGROUPS);
    ck_assert(1);


    conf_assert_na(LR_YRC_BANDWIDTH);
    conf_assert_set_uint64(LR_YRC_BANDWIDTH, G_GUINT64_CONSTANT(55));
    conf_assert_uint64_eq(LR_YRC_BANDWIDTH, G_GUINT64_CONSTANT(55));

    conf_assert_na(LR_YRC_THROTTLE);
    conf_assert_set_str(LR_YRC_THROTTLE, "test_throttle");
    conf_assert_str_eq(LR_YRC_THROTTLE, "test_throttle");

    conf_assert_na(LR_YRC_IP_RESOLVE);
    conf_assert_set_lripresolvetype(LR_YRC_IP_RESOLVE, LR_IPRESOLVE_V4);
    conf_assert_lripresolvetype_eq(LR_YRC_IP_RESOLVE, LR_IPRESOLVE_V4);


    conf_assert_na(LR_YRC_METADATA_EXPIRE);
    conf_assert_set_int64(LR_YRC_METADATA_EXPIRE, G_GINT64_CONSTANT(123));
    conf_assert_int64_eq(LR_YRC_METADATA_EXPIRE, G_GINT64_CONSTANT(123));

    conf_assert_na(LR_YRC_COST);
    conf_assert_set_int(LR_YRC_COST, 456);
    conf_assert_int_eq(LR_YRC_COST, 456);

    conf_assert_na(LR_YRC_PRIORITY);
    conf_assert_set_int(LR_YRC_PRIORITY, 789);
    conf_assert_int_eq(LR_YRC_PRIORITY, 789);


    conf_assert_na(LR_YRC_SSLCACERT);
    conf_assert_set_str(LR_YRC_SSLCACERT, "test_sslcacert");
    conf_assert_str_eq(LR_YRC_SSLCACERT, "test_sslcacert");

    conf_assert_na(LR_YRC_SSLVERIFY);
    conf_assert_set_boolean(LR_YRC_SSLVERIFY, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_SSLVERIFY);
    ck_assert(1);

    conf_assert_na(LR_YRC_SSLCLIENTCERT);
    conf_assert_set_str(LR_YRC_SSLCLIENTCERT, "test_sslclientcert");
    conf_assert_str_eq(LR_YRC_SSLCLIENTCERT, "test_sslclientcert");

    conf_assert_na(LR_YRC_SSLCLIENTKEY);
    conf_assert_set_str(LR_YRC_SSLCLIENTKEY, "test_sslclientkey");
    conf_assert_str_eq(LR_YRC_SSLCLIENTKEY, "test_sslclientkey");


    conf_assert_na(LR_YRC_DELTAREPOBASEURL);
    conf_assert_set_strv(LR_YRC_DELTAREPOBASEURL, "test_deltarepobaseurl", NULL);
    conf_assert_strv_eq(LR_YRC_DELTAREPOBASEURL, "test_deltarepobaseurl", NULL);


    conf_assert_na(LR_YRC_FAILOVERMETHOD);
    conf_assert_set_str(LR_YRC_FAILOVERMETHOD, "test_failovermethod");
    conf_assert_str_eq(LR_YRC_FAILOVERMETHOD, "test_failovermethod");

    conf_assert_na(LR_YRC_SKIP_IF_UNAVAILABLE);
    conf_assert_set_boolean(LR_YRC_SKIP_IF_UNAVAILABLE, TRUE);
    ck_assert(1);
    conf_assert_true(LR_YRC_SKIP_IF_UNAVAILABLE);
    ck_assert(1);

    // Write out modified config
    ret = lr_yum_repoconfs_save(confs, &tmp_err);
    ck_assert(!tmp_err);
    ck_assert(ret);

    // Cleanup resources
    lr_yum_repoconfs_free(confs);
    unlink(tmpfn);
}
END_TEST

Suite *
repoconf_suite(void)
{
    Suite *s = suite_create("repoconf");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_parse_repoconf_minimal);
    tcase_add_test(tc, test_parse_repoconf_big);
    tcase_add_test(tc, test_write_repoconf);
    suite_add_tcase(s, tc);
    return s;
}
