#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
    void *ptr = NULL;
    GError *tmp_err = NULL;
    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &ptr);
    ck_assert_msg(ret, "Getinfo failed for %d", option);
    fail_if(tmp_err);
    ck_assert_msg(ptr, "Not a True value (Option %d)", option);
}

#define conf_assert_true(option) \
            repoconf_assert_true(conf, (option))

static void
repoconf_assert_false(LrYumRepoConf *repoconf,
                      LrYumRepoConfOption option)
{
    void *ptr = NULL;
    GError *tmp_err = NULL;
    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &ptr);
    ck_assert_msg(ret, "Getinfo failed for %d", option);
    fail_if(tmp_err);
    ck_assert_msg(!ptr, "Not a NULL/0 value (Option %d)", option);
}

#define conf_assert_false(option) \
            repoconf_assert_false(conf, (option))

static void
repoconf_assert_na(LrYumRepoConf *repoconf,
                   LrYumRepoConfOption option)
{
    void *ptr = NULL;
    GError *tmp_err = NULL;
    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &ptr);
    ck_assert_msg(!ret, "Getinfo succeed for %d", option);
    fail_if(!tmp_err);
    ck_assert_int_eq(tmp_err->code, LRE_NOTSET);
    //ck_assert_msg(!ptr, "Not a NULL/0 value (Option %d)", option);
}

#define conf_assert_na(option) \
            repoconf_assert_na(conf, (option))

static void
repoconf_assert_str_eq(LrYumRepoConf *repoconf,
                       LrYumRepoConfOption option,
                       gchar *expected)
{
    gchar *str = NULL;
    GError *tmp_err = NULL;
    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &str);
    ck_assert_msg(ret, "Getinfo failed for %d", option);
    fail_if(tmp_err);
    ck_assert_str_eq(str, expected);
    g_free(str);
}

#define conf_assert_str_eq(option, expected) \
            repoconf_assert_str_eq(conf, (option), (expected))

static void
repoconf_assert_int_eq(LrYumRepoConf *repoconf,
                       LrYumRepoConfOption option,
                       intmax_t expected)
{
    long val;
    GError *tmp_err = NULL;
    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &val);
    ck_assert_msg(ret, "Getinfo failed for %d", option);
    fail_if(tmp_err);
    ck_assert_int_eq(val, expected);
}

#define conf_assert_int_eq(option, expected) \
            repoconf_assert_int_eq(conf, (option), (expected))

static void
repoconf_assert_strv_eq(LrYumRepoConf *repoconf,
                           LrYumRepoConfOption option,
                           ...)
{
    va_list args;
    GError *tmp_err = NULL;
    char **strv = NULL;

    gboolean ret = lr_yumrepoconf_getinfo(repoconf, &tmp_err, option, &strv);
    ck_assert_msg(ret, "Getinfo failed for %d", option);
    fail_if(tmp_err);

    ck_assert_msg(strv, "NULL isn't strv");

    va_start (args, option);
    gchar **strv_p = strv;
    for (; *strv_p; strv_p++) {
        gchar *s = va_arg (args, gchar*);
        ck_assert_str_eq(*strv_p, s);
    }

    ck_assert_msg((*strv_p == va_arg(args, gchar*)),
                  "Lengths of lists are not the same");

    va_end (args);
    g_strfreev(strv);
}

#define conf_assert_strv_eq(option, ...) \
            repoconf_assert_strv_eq(conf, (option), __VA_ARGS__)

START_TEST(test_repoconf_minimal)
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

    conf = list->data;
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

    // Test content of second repo config

    conf = list->next->data;
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

    conf_assert_na(LR_YRC_DELTAREPOBASEURL);

    lr_yum_repoconfs_free(confs);
}
END_TEST

START_TEST(test_repoconf_big)
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

    conf = list->data;
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

    conf_assert_int_eq(LR_YRC_BANDWIDTH, 1024*1024);
    conf_assert_str_eq(LR_YRC_THROTTLE, "50%");
    conf_assert_int_eq(LR_YRC_IP_RESOLVE, LR_IPRESOLVE_V6);

    conf_assert_int_eq(LR_YRC_METADATA_EXPIRE, 60*60*24*5);
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

    lr_yum_repoconfs_free(confs);
}
END_TEST

Suite *
repoconf_suite(void)
{
    Suite *s = suite_create("repoconf");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_repoconf_minimal);
    tcase_add_test(tc, test_repoconf_big);
    suite_add_tcase(s, tc);
    return s;
}
