from tests.base import TestCaseWithFlask, MOCKURL
from tests.servermock.server import app
import tests.servermock.yum_mock.config as config
import os
import os.path
import unittest
import tempfile
import shutil
import librepo

class TestCaseYumPackageDownloading(TestCaseWithFlask):
    application = app

#    @classmethod
#    def setUpClass(cls):
#        super(TestCaseYumPackageDownloading, cls).setUpClass()

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def test_download_package_01(self):
        """Just test download.
        Destination dir is specified by LRO_DESTDIR in handle"""
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_CHECKSUM, True)
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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_CHECKSUM, True)
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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.download(config.PACKAGE_01_01,
                   dest=self.tmpdir+"/",
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_via_metalink(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.METALINK_GOOD_01)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_via_mirrorlist(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_GOOD_01)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

    def test_download_package_with_baseurl(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.BADURL)
        baseurl = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
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
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def test_download_packages_00(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

        pkgs = []
        librepo.download_packages(pkgs)

    def test_download_packages_01(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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
        self.assertTrue(os.path.isfile(pkgs[1].local_path))

    def test_download_packages_one_url_is_bad_with_failfast(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

        pkgs = []
        pkgs.append(librepo.PackageTarget(config.PACKAGE_01_01,
                                          handle=h,
                                          dest=self.tmpdir))
        pkgs.append(librepo.PackageTarget("so_bad_url_of_foo_rpm.rpm",
                                          handle=h,
                                          dest=self.tmpdir))

        self.assertRaises(librepo.LibrepoException, librepo.download_packages,
                pkgs, failfast=True)

        # Err state is undefined, it could be error because of interruption
        # of could bo None because it was downloaded at one shot before
        # second download fails.

        #self.assertTrue(pkgs[0].err is not None)
        self.assertTrue(os.path.isfile(pkgs[0].local_path))

        self.assertTrue(pkgs[1].err is not None)
        self.assertTrue(os.path.isfile(pkgs[1].local_path))

    def test_download_packages_with_checksum_check(self):
        h = librepo.Handle()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, ["."])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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
        complete_url = "%s%s%s" % (MOCKURL,
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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h1.setopt(librepo.LRO_URLS, [url])
        h1.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

        url = "%s%s" % (MOCKURL, config.REPO_YUM_03_PATH)
        h2.setopt(librepo.LRO_URLS, [url])
        h2.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URLS, [url])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

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
