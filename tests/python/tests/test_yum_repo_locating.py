import os.path
import tempfile
import shutil
import unittest
import gpgme
import librepo

from tests.base import TestCase, TEST_DATA

# Repositories used in download tests
#REPOS_YUM = "tests/python/tests/servermock/yum_mock/static/"

REPO_YUM_01_PATH = TEST_DATA+"/repo_yum_01/"
REPO_YUM_02_PATH = TEST_DATA+"/repo_yum_02/"
PUB_KEY = TEST_DATA+"/key.pub"
MIRRORLIST = TEST_DATA+"/mirrorlist"
METALINK = TEST_DATA+"/metalink.xml"

class TestCaseYumRepoLocating(TestCase):

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")
        # Import public key into the temporary gpg keyring
        self._gnupghome = os.environ.get('GNUPGHOME')
        gpghome = os.path.join(self.tmpdir, "keyring")
        os.mkdir(gpghome, 0o700)
        os.environ['GNUPGHOME'] = gpghome
        self.ctx = gpgme.Context()
        self.ctx.import_(open(PUB_KEY, 'rb'))

    def tearDown(self):
        self.ctx.delete(self.ctx.get_key('22F2C4E9'))
        if self._gnupghome is None:
            os.environ.pop('GNUPGHOME')
        else:
            os.environ['GNUPGHOME'] = self._gnupghome
        shutil.rmtree(self.tmpdir)

    def test_read_mirrorlist(self):
        h = librepo.Handle()
        r = librepo.Result()
        h.setopt(librepo.LRO_MIRRORLIST, MIRRORLIST)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_FETCHMIRRORS, True)
        h.perform(r)
        self.assertEqual(h.mirrors, ['http://127.0.0.1:5000/yum/static/01/'])
        self.assertEqual(h.metalink, None)

    def test_read_metalink(self):
        h = librepo.Handle()
        r = librepo.Result()
        h.setopt(librepo.LRO_MIRRORLIST, METALINK)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_FETCHMIRRORS, True)
        h.perform(r)
        self.assertEqual(h.mirrors, ['http://127.0.0.1:5000/yum/static/01/'])
        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:5000/yum/static/01/repodata/repomd.xml',
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'})

    def test_read_metalink_of_local_repo(self):
        # At first, download whole repository
        # Check if local metalink.xml will be copied as well
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_01_PATH])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertFalse(yum_repo_downloaded["mirrorlist"])
        self.assertTrue(yum_repo_downloaded["metalink"])
        self.assertTrue(yum_repo_downloaded["metalink"].endswith("metalink.xml"))

        # Now try to read metalink of the repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [self.tmpdir])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        h.setopt(librepo.LRO_FETCHMIRRORS, True)
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:5000/yum/static/01/'])
        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:5000/yum/static/01/repodata/repomd.xml',
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'}
            )

    def test_locate_repo_01(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_01_PATH])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository and all its files
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [self.tmpdir])
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

        h.setopt(librepo.LRO_URLS, [REPO_YUM_02_PATH])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_GPGCHECK, False)
        h.perform(r)

        yum_repo_downloaded   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd_downloaded = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Now try to localize the existing repository and all its files
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [self.tmpdir])
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

        h.setopt(librepo.LRO_URLS, [REPO_YUM_01_PATH])
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

        h.setopt(librepo.LRO_URLS, [self.tmpdir])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_locate_incomplete_repo_01_2(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_01_PATH])
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

        h.setopt(librepo.LRO_URLS, [self.tmpdir])
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

    def test_locate_with_gpgcheck_enabled_but_without_signature(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_02_PATH])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_GPGCHECK, True)
        h.setopt(librepo.LRO_LOCAL, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_locate_with_metalink_and_mirrorlist_urls_set(self):
        # See: https://github.com/Tojaj/librepo/issues/41
        h = librepo.Handle()
        r = librepo.Result()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_02_PATH])
        # These bad URLs should not raise any error because
        # librepo shoudn't try to download them
        h.setopt(librepo.LRO_METALINKURL, "xyz://really-bad-url-sf987243kj.com")
        h.setopt(librepo.LRO_MIRRORLISTURL, "xyz://really-bad-url-mi-qcwmi.com")
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)
        h.perform(r)

    def test_locate_with_offline(self):
        h = librepo.Handle()
        h.urls = [REPO_YUM_02_PATH]
        h.metalinkurl = "xyz://really-bad-url-sf987243kj.com"
        h.mirrorlisturl = "xyz://really-bad-url-mi-qcwmi.com"
        h.repotype = librepo.LR_YUMREPO
        h.local = True
        h.offline = True
        # Offline and Local options shouldn't collide in any way
        h.perform()
