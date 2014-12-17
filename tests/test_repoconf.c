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

    ck_assert_str_eq(conf->id,      "minimal-repo-1");
    ck_assert_str_eq(conf->name,    "Minimal repo 1 - $basearch");
    ck_assert(conf->enabled);
    lr_assert_strv_eq(conf->baseurl, "http://m1.com/linux/$basearch", NULL);
    ck_assert(!conf->mirrorlist);
    ck_assert(!conf->metalink);

    ck_assert(!conf->mediaid);
    ck_assert(!conf->gpgkey);
    ck_assert(!conf->gpgcakey);
    ck_assert(!conf->exclude);
    ck_assert(!conf->include);

    ck_assert(!conf->fastestmirror);
    ck_assert(!conf->proxy);
    ck_assert(!conf->proxy_username);
    ck_assert(!conf->proxy_password);
    ck_assert(!conf->username);
    ck_assert(!conf->password);

    ck_assert(!conf->gpgcheck);
    ck_assert(!conf->repo_gpgcheck);
    ck_assert(conf->enablegroups);

    ck_assert_int_eq(conf->bandwidth, LR_YUMREPOCONF_BANDWIDTH_DEFAULT);
    ck_assert(!conf->throttle);
    ck_assert_int_eq(conf->ip_resolve, LR_YUMREPOCONF_IP_RESOLVE_DEFAULT);

    ck_assert_int_eq(conf->metadata_expire, LR_YUMREPOCONF_METADATA_EXPIRE_DEFAULT);
    ck_assert_int_eq(conf->cost,            LR_YUMREPOCONF_COST_DEFAULT);
    ck_assert_int_eq(conf->priority,        LR_YUMREPOCONF_PRIORITY_DEFAULT);

    ck_assert(!conf->sslcacert);
    ck_assert(conf->sslverify);
    ck_assert(!conf->sslclientcert);
    ck_assert(!conf->sslclientkey);

    ck_assert(!conf->deltarepobaseurl);

    // Test content of second repo config

    conf = list->next->data;
    fail_if(!conf);

    ck_assert_str_eq(conf->id,      "minimal-repo-2");
    ck_assert_str_eq(conf->name,    "Minimal repo 2 - $basearch");
    ck_assert(conf->enabled);
    lr_assert_strv_eq(conf->baseurl, "http://m2.com/linux/$basearch", NULL);
    ck_assert(!conf->mirrorlist);
    ck_assert(!conf->metalink);

    ck_assert(!conf->mediaid);
    ck_assert(!conf->gpgkey);
    ck_assert(!conf->gpgcakey);
    ck_assert(!conf->exclude);
    ck_assert(!conf->include);

    ck_assert(!conf->fastestmirror);
    ck_assert(!conf->proxy);
    ck_assert(!conf->proxy_username);
    ck_assert(!conf->proxy_password);
    ck_assert(!conf->username);
    ck_assert(!conf->password);

    ck_assert(!conf->gpgcheck);
    ck_assert(!conf->repo_gpgcheck);
    ck_assert(conf->enablegroups);

    ck_assert_int_eq(conf->bandwidth, LR_YUMREPOCONF_BANDWIDTH_DEFAULT);
    ck_assert(!conf->throttle);
    ck_assert_int_eq(conf->ip_resolve, LR_YUMREPOCONF_IP_RESOLVE_DEFAULT);

    ck_assert_int_eq(conf->metadata_expire, LR_YUMREPOCONF_METADATA_EXPIRE_DEFAULT);
    ck_assert_int_eq(conf->cost,            LR_YUMREPOCONF_COST_DEFAULT);
    ck_assert_int_eq(conf->priority,        LR_YUMREPOCONF_PRIORITY_DEFAULT);

    ck_assert(!conf->sslcacert);
    ck_assert(conf->sslverify);
    ck_assert(!conf->sslclientcert);
    ck_assert(!conf->sslclientkey);

    ck_assert(!conf->deltarepobaseurl);


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

    ck_assert_str_eq(conf->id,      "big-repo");
    ck_assert_str_eq(conf->name,    "Maxi repo - $basearch");
    ck_assert(conf->enabled);
    lr_assert_strv_eq(conf->baseurl,
                      "http://foo1.org/pub/linux/$releasever/$basearch/os/",
                      "ftp://ftp.foo2/pub/linux/$releasever/$basearch/os/",
                      "https://foo3.org/pub/linux/",
                      NULL);
    ck_assert_str_eq(conf->mirrorlist,"http://foo1.org/mirrorlist");
    ck_assert_str_eq(conf->metalink,  "https://foo1.org/metalink?repo=linux-$releasever&arch=$basearch");

    ck_assert_str_eq(conf->mediaid,     "0");
    lr_assert_strv_eq(conf->gpgkey,
                      "https://foo1.org/linux/foo_signing_key.pub",
                      "https://foo2.org/linux/foo_signing_key.pub",
                      NULL);
    lr_assert_strv_eq(conf->gpgcakey,   "https://foo1.org/linux/ca_key.pub", NULL);
    lr_assert_strv_eq(conf->exclude,    "package_1", "package_2", NULL);
    lr_assert_strv_eq(conf->include,    "package_a", "package_b", NULL);

    ck_assert(conf->fastestmirror);
    ck_assert_str_eq(conf->proxy,           "socks5://127.0.0.1:5000");
    ck_assert_str_eq(conf->proxy_username,  "proxyuser");
    ck_assert_str_eq(conf->proxy_password,  "proxypass");
    ck_assert_str_eq(conf->username,        "user");
    ck_assert_str_eq(conf->password,        "pass");

    ck_assert(conf->gpgcheck);
    ck_assert(conf->repo_gpgcheck);
    ck_assert(conf->enablegroups);

    ck_assert_int_eq(conf->bandwidth,   1024*1024);
    ck_assert_str_eq(conf->throttle,    "50%");
    ck_assert_int_eq(conf->ip_resolve,  LR_IPRESOLVE_V6);

    ck_assert_int_eq(conf->metadata_expire, 60*60*24*5);
    ck_assert_int_eq(conf->cost,            500);
    ck_assert_int_eq(conf->priority,        10);

    ck_assert_str_eq(conf->sslcacert,       "file:///etc/ssl.cert");
    ck_assert(conf->sslverify);
    ck_assert_str_eq(conf->sslclientcert,   "file:///etc/client.cert");
    ck_assert_str_eq(conf->sslclientkey,    "file:///etc/client.key");

    lr_assert_strv_eq(conf->deltarepobaseurl,
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
