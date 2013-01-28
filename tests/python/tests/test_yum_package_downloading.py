from base import TestCaseWithFlask, MOCKURL
from servermock.server import app
import servermock.yum_mock.config as config
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
        h.setopt(librepo.LRO_URL, url)
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
        h.setopt(librepo.LRO_URL, url)
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
        h.setopt(librepo.LRO_URL, url)
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
        h.setopt(librepo.LRO_URL, url)
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
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.download(config.PACKAGE_01_01,
                   checksum=config.PACKAGE_01_01_SHA256,
                   checksum_type=librepo.CHECKSUM_SHA256,
                   base_url=baseurl)

        pkg = os.path.join(self.tmpdir, config.PACKAGE_01_01)
        self.assertTrue(os.path.isfile(pkg))

