import os.path
import tempfile
import shutil
import unittest
import librepo

# Repositories used in download tests
#REPOS_YUM = "tests/python/tests/servermock/yum_mock/static/"

REPOS_YUM = "tests/repos/"

REPO_YUM_01_PATH = REPOS_YUM+"repo_yum_01/"
REPO_YUM_02_PATH = REPOS_YUM+"repo_yum_02/"

class TestCaseYumRepoDownloading(unittest.TestCase):

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")

    def tearDown(self):
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


