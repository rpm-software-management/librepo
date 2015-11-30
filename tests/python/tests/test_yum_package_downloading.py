import os
import shutil
import os.path
import librepo
import hashlib
import unittest
import tempfile
import xattr

import tests.servermock.yum_mock.config as config

from tests.base import TestCaseWithFlask
from tests.servermock.server import app


class TestCaseYumPackageDownloading(TestCaseWithFlask):

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-", dir="./")

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def test_download_package_01(self):
        """Just test download.
        Destination dir is specified by LRO_DESTDIR in handle"""
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.download(config.PACKAGE_01_01)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_02(self):
        """Download with checksum check.
        Destination file is specified by full path
        in dest option of download() method and
        checksum type is specified by string"""
        h = librepo.Handle()

        destination = os.path.join(self.tmpdir, "pkg.rpm")

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.checksum = True
        h.download(config.PACKAGE_01_01,
                   dest=destination,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type="sha256")

        self.assertTrue(os.path.isfile(destination))

    def test_download_package_03(self):
        """Download with checksum check.
        Destination dir is not specified and cwd shoud
        be used"""
        cwd = os.getcwd()

        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.checksum = True
        os.chdir(self.tmpdir)
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)
        os.chdir(cwd)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_04(self):
        """Download with checksum check.
        Destination dir is specified in dest option
        of download() method"""

        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.checksum = True
        h.download(config.PACKAGE_01_01,
                   dest=self.tmpdir+"/",
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_via_metalink(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_via_mirrorlist(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_with_baseurl(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.BADURL)
        baseurl = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256,
                   base_url=baseurl)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

class TestCaseYumPackagesDownloading(TestCaseWithFlask):
    application = app

#    @classmethod
#    def setUpClass(cls):
#        super(TestCaseYumPackageDownloading, cls).setUpClass()

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-", dir="./")

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def test_packagetarget_sanity_00(self):
        t = librepo.PackageTarget("foo")

        self.assertEqual(t.relative_url, "foo")
        self.assertEqual(t.dest, None)
        self.assertEqual(t.checksum_type, librepo.CHECKSUM_UNKNOWN)
        self.assertEqual(t.checksum, None)
        self.assertEqual(t.expectedsize, 0)
        self.assertEqual(t.base_url, None)
        self.assertEqual(t.resume, False)
        self.assertEqual(t.progresscb, None)
        self.assertEqual(t.cbdata, None)
        self.assertEqual(t.handle, None)
        self.assertEqual(t.endcb, None)
        self.assertEqual(t.mirrorfailurecb, None)

    def test_packagetarget_sanity_01(self):

        h = librepo.Handle()
        cbdata = {"a": "b"}
        def progresscb(a, b, c): return 0
        def endcb(a): return
        def mirrorfailurecb(a, b): return 0;

        t = librepo.PackageTarget("foo",
                                  dest="bar",
                                  checksum_type=librepo.CHECKSUM_SHA512,
                                  checksum="xxx",
                                  expectedsize=123,
                                  base_url="basefoo",
                                  resume=True,
                                  progresscb=progresscb,
                                  cbdata=cbdata,
                                  handle=h,
                                  endcb=endcb,
                                  mirrorfailurecb=mirrorfailurecb)

        self.assertEqual(t.relative_url, "foo")
        self.assertEqual(t.dest, "bar")
        self.assertEqual(t.checksum_type, librepo.CHECKSUM_SHA512)
        self.assertEqual(t.checksum, "xxx")
        self.assertEqual(t.expectedsize, 123)
        self.assertEqual(t.base_url, "basefoo")
        self.assertEqual(t.resume, True)
        self.assertEqual(t.progresscb, progresscb)
        self.assertEqual(t.cbdata, cbdata)
        self.assertEqual(t.handle, h)
        self.assertEqual(t.endcb, endcb)
        self.assertEqual(t.mirrorfailurecb, mirrorfailurecb)

    def test_download_packages_00(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        librepo.download_packages(pkgs)

    def test_download_packages_01(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs)

        for pkg in pkgs:
            self.assertTrue(pkg.err is None)
            self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_02(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        dest = os.path.join(self.tmpdir, "foo-haha-lol.rpm")
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=dest))

        librepo.download_packages(pkgs)

        for pkg in pkgs:
            self.assertTrue(pkg.err is None)
            self.assertTrue(os.path.isfile(pkg.local_path))

        self.assertTrue(os.path.isfile(dest))

    def test_download_packages_02_with_failfast(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        dest = os.path.join(self.tmpdir, "foo-haha-lol.rpm")
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=dest))

        librepo.download_packages(pkgs, failfast=True)

        for pkg in pkgs:
            self.assertTrue(pkg.err is None)
            self.assertTrue(os.path.isfile(pkg.local_path))

        self.assertTrue(os.path.isfile(dest))

    def test_download_packages_one_url_is_bad_01(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget("so_bad_url_of_foo_rpm.rpm",
                                          handle=h,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs)

        self.assertTrue(pkgs[0].err is None)
        self.assertTrue(os.path.isfile(pkgs[0].local_path))

        self.assertTrue(pkgs[1].err is not None)
        self.assertFalse(os.path.isfile(pkgs[1].local_path))

    def test_download_packages_one_url_is_bad_with_failfast(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget("so_bad_url_of_foo_rpm.rpm",
                                          handle=h,
                                          dest=self.tmpdir))

        self.assertRaises(librepo.LibrepoException, librepo.download_packages,
                pkgs, failfast=True)

        # Err state of first download is undefined,
        # it could be error because of interruption
        # or could be None (when the download was successful)
        # in case that it was downloaded at one shot before
        # the second download fails.

        # XXX: TODO
        #self.assertTrue(os.path.isfile(pkgs[0].local_path))

        self.assertTrue(pkgs[1].err is not None)
        self.assertFalse(os.path.isfile(pkgs[1].local_path))

    def test_download_packages_with_checksum_check(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          checksum_type=librepo.SHA256,
                                          checksum=config.PACKAGE_01_01_SHA256))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertEqual(pkg.relative_url, config.PACKAGE_01_01)
        self.assertEqual(pkg.dest, self.tmpdir)
        self.assertEqual(pkg.base_url, None)
        self.assertEqual(pkg.checksum_type, librepo.SHA256)
        self.assertEqual(pkg.checksum, config.PACKAGE_01_01_SHA256)
        self.assertEqual(pkg.resume, 0)
        self.assertTrue(os.path.isfile(pkg.local_path))
        self.assertEqual(pkg.local_path, os.path.join(self.tmpdir,
                                    os.path.basename(config.PACKAGE_01_01)))
        self.assertTrue(pkg.err is None)

    def test_download_packages_with_bad_checksum(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          checksum_type=librepo.SHA256,
                                          checksum="badchecksum"))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertTrue(pkg.err)

    def test_download_packages_with_bad_checksum_with_failfast(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          checksum_type=librepo.SHA256,
                                          checksum="badchecksum"))

        self.assertRaises(librepo.LibrepoException, librepo.download_packages,
                          pkgs, failfast=True)

        pkg = pkgs[0]
        self.assertTrue(pkg.err)

    def test_download_packages_with_baseurl(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = ["."]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          base_url=url))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_resume(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          resume=False))

        librepo.download_packages(pkgs)
        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))

        # Again with resume on
        # Because the package already exists and checksum matches,
        # then download should not be performed!
        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          resume=True,
                                          checksum_type=librepo.SHA256,
                                          checksum=config.PACKAGE_01_01_SHA256))

        librepo.download_packages(pkgs)
        pkg = pkgs[0]
        self.assertEqual(pkg.err, "Already downloaded")
        self.assertTrue(os.path.isfile(pkg.local_path))

        ## Failfast ignore this type of error (Already downloaded error)
        librepo.download_packages(pkgs, failfast=True)
        pkg = pkgs[0]
        self.assertEqual(pkg.err, "Already downloaded")
        self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_callback(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        cbdata = {'called': 0}
        def cb(cbdata, total, downloaded):
            cbdata["called"] += 1
            cbdata["total"] = total
            cbdata["downloaded"] = downloaded

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir,
                                          progresscb=cb,
                                          cbdata=cbdata))

        librepo.download_packages(pkgs)
        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(pkg.progresscb == cb)
        self.assertTrue(pkg.cbdata == cbdata)
        self.assertTrue(os.path.isfile(pkg.local_path))
        self.assertTrue(cbdata["called"] > 0)
        self.assertTrue(cbdata["downloaded"] > 0)
        self.assertTrue(cbdata["total"] > 0)
        self.assertEqual(cbdata["downloaded"], cbdata["total"])

    def test_download_packages_without_handle(self):
        complete_url = "%s%s%s" % (self.MOCKURL,
                                   config.REPO_YUM_01_PATH,
                                   config.PACKAGE_01_01)

        pkgs = []
        pkgs.append(librepo.PackageTarget(complete_url,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_from_different_repos(self):
        h1 = librepo.Handle()
        h2 = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h1.urls = [url]
        h1.repotype = librepo.LR_YUMREPO

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_03_PATH)
        h2.urls = [url]
        h2.repotype = librepo.LR_YUMREPO

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h1,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_03_01,
                                          handle=h2,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs)
        pkg = pkgs[0]
        self.assertTrue(pkg.handle == h1)
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))
        pkg = pkgs[1]
        self.assertTrue(pkg.handle == h2)
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_expectedsize(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        expectedsize = 1057084

        pkgs = []
        pkgs.append(h.new_packagetarget(config.PACKAGE_01_01,
                                        dest=self.tmpdir,
                                        expectedsize=expectedsize))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertEqual(pkg.expectedsize, expectedsize)
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_fastestmirror_enabled_1(self):
        h = librepo.Handle()

        cbdata = {
            "called": False,
        }
        def fastestmirrorstatuscallback(userdata, stage, data):
            cbdata["called"] = True

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fastestmirror = True
        h.fastestmirrorcb = fastestmirrorstatuscallback

        h.download(config.PACKAGE_01_01)

        pkgs = []
        pkgs.append(h.new_packagetarget(config.PACKAGE_01_01,
                                        dest=self.tmpdir))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))
        # There is only one mirror, fastestmirror
        # detection should be skiped
        self.assertFalse(cbdata["called"])


    def test_download_packages_with_fastestmirror_enabled_2(self):
        h = librepo.Handle()

        cbdata = {
            "called": False,
            "detection": False,
        }

        def fastestmirrorstatuscallback(cbdata, stage, data):
            cbdata["called"] = True
            if stage == librepo.FMSTAGE_DETECTION:
                cbdata["detection"] = True

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url, "http://foobarblabla"]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fastestmirror = True
        h.fastestmirrordata = cbdata
        h.fastestmirrorcb = fastestmirrorstatuscallback

        h.download(config.PACKAGE_01_01)

        pkgs = []
        pkgs.append(h.new_packagetarget(config.PACKAGE_01_01,
                                        dest=self.tmpdir))

        librepo.download_packages(pkgs)

        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))
        # There is only one mirror, fastestmirror
        # detection should be skiped
        self.assertTrue(cbdata["called"])
        self.assertTrue(cbdata["detection"])

    def test_download_packages_from_different_mirrors_1(self):
        cbdata = {
            "failed_urls" : [],
        }

        def mirrorfailurecb(userdata, msg, url):
            cbdata["failed_urls"].append(url)
            return None

        h = librepo.Handle()

        url1 = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        url2 = "%s%s" % (self.MOCKURL, config.REPO_YUM_03_PATH)
        url3 = "%s%s" % (self.MOCKURL, config.REPO_YUM_04_PATH)

        h.mirrorlist = "%s%s" % (self.MOCKURL, config.METALINK_MIRRORS_01)
        h.repotype = librepo.LR_YUMREPO
        h.maxparalleldownloads = 1
        h.fastestmirror = True

        pkgs = []
        for P in [config.PACKAGE_01_01, config.PACKAGE_03_01, config.PACKAGE_04_01]:
            pkgs.append(librepo.PackageTarget(P,
                                              handle=h,
                                              dest=self.tmpdir,
                                              mirrorfailurecb=mirrorfailurecb))

        librepo.download_packages(pkgs, failfast=True)

        # the mirror list should be 1, 3, 4. The metalink file
        # defines preference order. This is the order in which
        # the URLs are listed in the file
        self.assertEquals(h.mirrors, [url1, url2, url3])

        # YUM 01 contains P_01
        # YUM 03 contains P_03
        # YUM_04 contains P_04
        # Mirror order of preference is 01 03, 04
        # When a package fails to download, librepo moves to the next mirror;
        # after a successfull download of the package librepo should continue trying
        # to download the remaining packages from the first mirror. e.g.
        # always try to download from the fastest mirror containing the package

        # Expected download sequence (restart from fastest mirror):
        # - P_01 from YUM_01 - PASS
        # - P_03 from YUM_01 - FAIL
        # - P_03 from YUM_03 - PASS
        # - P_04 from YUM_04 - FAIL
        # - P_04 from YUM_03 - FAIL
        # - P_04 from YUM_04 - PASS
        self.assertEquals(len(cbdata["failed_urls"]), 3)
        self.assertEquals(cbdata["failed_urls"][0], url1+config.PACKAGE_03_01)
        self.assertEquals(cbdata["failed_urls"][1], url1+config.PACKAGE_04_01)
        self.assertEquals(cbdata["failed_urls"][2], url2+config.PACKAGE_04_01)

        # assert that all packages have been downloaded
        for pkg in pkgs:
            self.assertTrue(pkg.err is None)
            self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_bad_first_mirror_1(self):
        h = librepo.Handle()

        url1 = "%s%s" % (self.MOCKURL, config.REPO_YUM_02_PATH)
        url2 = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        url3 = "%s%s" % (self.MOCKURL, config.REPO_YUM_03_PATH)
        h.urls = [url1, url2, url3]
        h.repotype = librepo.LR_YUMREPO
        h.maxparalleldownloads = 1
        h.allowedmirrorfailures = 1

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_03_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))

        # Should raise an error, because, the first two mirror
        # should be disabled during download of the first package
        # and when is the time to download the second package,
        # the repo that contains the package is disabled yet.
        self.assertRaises(librepo.LibrepoException, librepo.download_packages,
                pkgs, failfast=True)

        self.assertTrue(pkgs[0].err is None)
        self.assertTrue(os.path.isfile(pkgs[0].local_path))
        self.assertTrue(pkgs[1].err is not None)
        self.assertFalse(os.path.isfile(pkgs[1].local_path))

    def test_download_packages_with_resume_02(self):
        # If download that should be resumed fails,
        # the original file should not be modified or deleted
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO

        fn = os.path.join(self.tmpdir, "package.rpm")

        # Download first 10 bytes of the package
        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=fn,
                                          resume=False,
                                          byterangeend=9))

        librepo.download_packages(pkgs)
        pkg = pkgs[0]
        self.assertTrue(pkg.err is None)
        self.assertTrue(os.path.isfile(pkg.local_path))
        self.assertEqual(os.path.getsize(pkg.local_path), 10)
        fchksum = hashlib.md5(open(pkg.local_path, "rb").read()).hexdigest()

        # Mark the file as it was downloaded by Librepo
        # Otherwise librepo refuse to resume
        try:
            xattr.setxattr(pkg.local_path,
                           "user.Librepo.DownloadInProgress".encode("utf-8"),
                           "".encode("utf-8"))
        except IOError as err:
            if err.errno == 95:
                self.skipTest('extended attributes are not supported')
            raise

        # Now try to resume from bad URL
        pkgs = []
        pkgs.append(librepo.PackageTarget("bad_path.rpm",
                                          handle=h,
                                          dest=fn,
                                          resume=True,
                                          checksum_type=librepo.SHA256,
                                          checksum=config.PACKAGE_01_01_SHA256))

        # Download should fail (path is bad, the file doesn't exist)
        self.assertRaises(librepo.LibrepoException, librepo.download_packages,
                pkgs, failfast=True)

        # The package should exists (should not be removed or changed)
        pkg = pkgs[0]
        self.assertTrue(pkg.err)
        self.assertTrue(os.path.isfile(pkg.local_path))
        self.assertEqual(os.path.getsize(pkg.local_path), 10)
        fchksum_new = hashlib.md5(open(pkg.local_path, "rb").read()).hexdigest()
        self.assertEqual(fchksum, fchksum_new)

    def test_download_packages_mirror_penalization_01(self):

        # This test is useful for mirror penalization testing
        # with debug output on:
        # LIBREPO_DEBUG_ADAPTIVEMIRRORSORTING="1"
        #
        # You can use it to check adaptive mirror sorting behavior
        #
        # E.g. from librepo checkout dir:
        # LIBREPO_DEBUG_ADAPTIVEMIRRORSORTING="1" LIBREPO_DEBUG="1" PYTHONPATH=`readlink -f ./build/librepo/python/python2/` nosetests -s -v tests/python/tests/test_yum_package_downloading.py:TestCaseYumPackagesDownloading.test_download_packages_mirror_penalization_01

        h = librepo.Handle()

        url1 = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        url2 = "%s%s" % (self.MOCKURL, config.REPO_YUM_03_PATH)
        url3 = "%s%s" % (self.MOCKURL, config.REPO_YUM_04_PATH)
        h.urls = [url1, url2, url3]
        h.repotype = librepo.LR_YUMREPO
        h.maxparalleldownloads = 1
        h.allowedmirrorfailures = -1
        h.adaptivemirrorsorting = True

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_03_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_03_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_04_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs, failfast=False)

        for pkg in pkgs:
            self.assertTrue(pkg.err is None)
            self.assertTrue(os.path.isfile(pkg.local_path))

    def test_download_packages_with_offline_enabled_01(self):
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)

        h = librepo.Handle()
        h.urls = [url]
        h.repotype = librepo.YUMREPO
        h.offline = True

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))

        librepo.download_packages(pkgs)

        # Offline is True, no package should be downloaded successfully
        for pkg in pkgs:
            self.assertTrue(pkg.err)

        h.offline = False

        librepo.download_packages(pkgs)
        # Offline is False, the package should be downloaded successfully
        for pkg in pkgs:
            self.assertFalse(pkg.err)

    def test_download_packages_with_offline_enabled_02(self):
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)

        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.offline = True
        h.urls = [url]

        # 1) Try to download a package with only rel path specified
        # (mirrors from handle will be used)
        pkgs = [librepo.PackageTarget(config.PACKAGE_01_01,
                                      handle=h,
                                      dest=os.path.join(self.tmpdir, "01"))]
        self.assertRaises(librepo.LibrepoException,
                          librepo.download_packages, pkgs, failfast=True)
        self.assertTrue(pkgs[0].err)

        # Remove mirrors from handle (the one which was inserted via urls opt)
        h.urls = []

        # 2) Try to download a package with full path specified
        full_url = os.path.join(url, config.PACKAGE_01_01)
        pkgs = [librepo.PackageTarget(full_url,
                                      handle=h,
                                      dest=os.path.join(self.tmpdir, "01"))]
        self.assertRaises(librepo.LibrepoException,
                          librepo.download_packages, pkgs, failfast=True)
        self.assertTrue(pkgs[0].err)

        # 3) Try to download a package with rel path and base url specified
        pkgs = [librepo.PackageTarget(config.PACKAGE_01_01,
                                      base_url=url,
                                      handle=h,
                                      dest=os.path.join(self.tmpdir, "01"))]
        self.assertRaises(librepo.LibrepoException,
                          librepo.download_packages, pkgs, failfast=True)
        self.assertTrue(pkgs[0].err)

