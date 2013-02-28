import os.path
import tempfile
import shutil
import unittest
import gpgme
import librepo

from base import TestCase, TEST_DATA

# Repositories used in download tests
#REPOS_YUM = "tests/python/tests/servermock/yum_mock/static/"

REPO_YUM_01_PATH = TEST_DATA+"/repo_yum_01/"
REPO_YUM_02_PATH = TEST_DATA+"/repo_yum_02/"
PUB_KEY = TEST_DATA+"/key.pub"

class TestCaseYumRepoLocating(TestCase):

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")
        # Import public key into the temporary gpg keyring
        self._gnupghome = os.environ.get('GNUPGHOME')
        gpghome = os.path.join(self.tmpdir, "keyring")
        os.mkdir(gpghome, 0700)
        os.environ['GNUPGHOME'] = gpghome
        self.ctx = gpgme.Context()
        self.ctx.import_(open(PUB_KEY))

    def tearDown(self):
        self.ctx.delete(self.ctx.get_key('22F2C4E9'))
        if self._gnupghome is None:
            os.environ.pop('GNUPGHOME')
        else:
            os.environ['GNUPGHOME'] = self._gnupghome
        shutil.rmtree(self.tmpdir)

    def test_locate_repo_01(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository and all its files
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, self.tmpdir)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Compare results
        yum_repo_downloaded["url"] = None
        self.assertEqual(yum_repo, yum_repo_downloaded)
        self.assertEqual(yum_repomd, yum_repomd_downloaded)

    def test_locate_repo_02(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, REPO_YUM_02_PATH)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository and all its files
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, self.tmpdir)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Compare results
        yum_repo_downloaded["url"] = None
        self.assertEqual(yum_repo, yum_repo_downloaded)
        self.assertEqual(yum_repomd, yum_repomd_downloaded)

    def test_locate_incomplete_repo_01(self):
        # At first, download only some files from the repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.setopt(librepo.LRO_YUMDLIST, ["primary"])
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, self.tmpdir)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_locate_incomplete_repo_01_2(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.setopt(librepo.LRO_YUMDLIST, ["primary"])
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository and all its files
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URL, self.tmpdir)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        h.setopt(librepo.LRO_IGNOREMISSING, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Compare results
        yum_repo_downloaded["url"] = None
        self.assertEqual(yum_repo, yum_repo_downloaded)
        self.assertEqual(yum_repomd, yum_repomd_downloaded)


