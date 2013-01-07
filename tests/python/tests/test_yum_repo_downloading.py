from base import TestCaseWithFlask, MOCKURL
from servermock.server import app
import servermock.yum_mock.config as config
import os.path
import unittest
import tempfile
import shutil
import librepo

class TestCaseYumRepoDownloading(TestCaseWithFlask):
    application = app

#    @classmethod
#    def setUpClass(cls):
#        super(TestCaseYumRepoDownloading, cls).setUpClass()

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="librepotest-")

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def test_download_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertEqual(yum_repo,
            { #'deltainfo': None,
              'destdir': self.tmpdir,
              'filelists': self.tmpdir+'/repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
              'filelists_db': self.tmpdir+'/repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
              #'group': None,
              #'group_gz': None,
              #'origin': None,
              'other': self.tmpdir+'/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
              'other_db': self.tmpdir+'/repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
              #'prestodelta': None,
              'primary': self.tmpdir+'/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
              'primary_db': self.tmpdir+'/repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
              'repomd': self.tmpdir+'/repodata/repomd.xml',
              #'updateinfo': None,
              'url': url}
        )

        self.assertEqual(yum_repomd,
            {   'content_tags': [],
                #'deltainfo': None,
                'distro_tags': [],
                'filelists': {
                    'checksum': 'aeca08fccd3c1ab831e1df1a62711a44ba1922c9',
                    'checksum_open': '52d30ae3162ca863c63c345ffdb7f0e10c1414a5',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 0L,
                    'location_href': 'repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                    'size': 43310L,
                    'size_open': 735088L,
                    'timestamp': 1347459930L},
                'filelists_db': {
                    'checksum': '4034dcea76c94d3f7a9616779539a4ea8cac288f',
                    'checksum_open': '949c6b7b605b2bc66852630c841a5003603ca5b2',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10L,
                    'location_href': 'repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                    'size': 22575L,
                    'size_open': 201728L,
                    'timestamp': 1347459931L},
                #'group': None,
                #'group_gz': None,
                #'origin': None,
                'other': {
                    'checksum': 'a8977cdaa0b14321d9acfab81ce8a85e869eee32',
                    'checksum_open': '4b5b8874fb233a626b03b3260a1aa08dce90e81a',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 0L,
                    'location_href': 'repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                    'size': 807L,
                    'size_open': 1910L,
                    'timestamp': 1347459930L},
                'other_db': {
                    'checksum': 'fd96942c919628895187778633001cff61e872b8',
                    'checksum_open': 'c5262f62b6b3360722b9b2fb5d0a9335d0a51112',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10L,
                    'location_href': 'repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                    'size': 1407L,
                    'size_open': 8192L,
                    'timestamp': 1347459931L},
                #'prestodelta': None,
                'primary': {
                    'checksum': '4543ad62e4d86337cd1949346f9aec976b847b58',
                    'checksum_open': '68457ceb8e20bda004d46e0a4dfa4a69ce71db48',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 0L,
                    'location_href': 'repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                    'size': 936L,
                    'size_open': 3385L,
                    'timestamp': 1347459930L},
                'primary_db': {
                    'checksum': '735cd6294df08bdf28e2ba113915ca05a151118e',
                    'checksum_open': 'ba636386312e1b597fc4feb182d04c059b2a77d5',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10L,
                    'location_href': 'repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                    'size': 2603L,
                    'size_open': 23552L,
                    'timestamp': 1347459931L},
                'repo_tags': [],
                'revision': '1347459931',
                #'updateinfo': None
                }
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_02_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertEqual(yum_repo,
            {'deltainfo': self.tmpdir+'/repodata/32d3307b672abf7356061912fa3dc9b54071c03a75c671111c1c8daf5ed1eb7e-deltainfo.xml.gz',
             'destdir': self.tmpdir,
             'filelists': self.tmpdir+'/repodata/2431efa18b5de6bfddb87da2a526362108226752d46ef3a298cd4bf39ba16b1d-filelists.xml.gz',
             'filelists_db': self.tmpdir+'/repodata/5b37f89f9f4474801ec5f23dc30d3d6cf9cf663cb75a6656aaa864a041836ffe-filelists.sqlite.bz2',
             'group': self.tmpdir+'/repodata/5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e-comps.xml',
             'group_gz': self.tmpdir+'/repodata/c395ae7d8a9117f4e81aa23e37fb9da9865b50917f5f701b50d422875bb0cb14-comps.xml.gz',
             'origin': self.tmpdir+'/repodata/c949d2b2371fab1a03d03b41057004caf1133a56e4c9236f63b3163ad358c941-pkgorigins.gz',
             'other': self.tmpdir+'/repodata/76b2cfb04531a66e382f187e6a7c90905940d2b2f315b7fd738b839887d83c35-other.xml.gz',
             'other_db': self.tmpdir+'/repodata/705a58b0e169bf1d2ade8e4aacc515086644ce16cee971906f920c798c5b17d0-other.sqlite.bz2',
             'prestodelta': self.tmpdir+'/repodata/26e351e1a38eb1524574e86ab130ea4db780aa1a4a8bb741d37595ed203f931c-prestodelta.xml.gz',
             'primary': self.tmpdir+'/repodata/5a8e6bbb940b151103b3970a26e32b8965da9e90a798b1b80ee4325308149d8d-primary.xml.gz',
             'primary_db': self.tmpdir+'/repodata/a09c42730c03b0d5defa3fd9213794c49e9bafbc67acdd8d4e87a2adf30b8752-primary.sqlite.bz2',
             'repomd': self.tmpdir+'/repodata/repomd.xml',
             'updateinfo': self.tmpdir+'/repodata/65c4f66e2808d328890505c3c2f13bb35a96f457d1c21a6346191c4dc07e6080-updateinfo.xml.gz',
             'url': url}
        )

        self.assertEqual(yum_repomd,
            {'content_tags': ['binary-i386'],
             'deltainfo': {'checksum': '32d3307b672abf7356061912fa3dc9b54071c03a75c671111c1c8daf5ed1eb7e',
                           'checksum_open': '8a35a38aef926fd88f479f03a9a22e1ab7aa8bd1aeaa9d05cd696f101eee2846',
                           'checksum_open_type': 'sha256',
                           'checksum_type': 'sha256',
                           'db_version': 0L,
                           'location_href': 'repodata/32d3307b672abf7356061912fa3dc9b54071c03a75c671111c1c8daf5ed1eb7e-deltainfo.xml.gz',
                           'size': 300L,
                           'size_open': 492L,
                           'timestamp': 1355335029L},
             'distro_tags': [('cpe:/o:fedoraproject:fedora:17', 'r')],
             'filelists': {'checksum': '2431efa18b5de6bfddb87da2a526362108226752d46ef3a298cd4bf39ba16b1d',
                           'checksum_open': 'afa4a01d7a692ab8105a39fed5535b5011f0c68de0efbc98f9d6ffea36de85fe',
                           'checksum_open_type': 'sha256',
                           'checksum_type': 'sha256',
                           'db_version': 0L,
                           'location_href': 'repodata/2431efa18b5de6bfddb87da2a526362108226752d46ef3a298cd4bf39ba16b1d-filelists.xml.gz',
                           'size': 43338L,
                           'size_open': 735112L,
                           'timestamp': 1355393567L},
             'filelists_db': {'checksum': '5b37f89f9f4474801ec5f23dc30d3d6cf9cf663cb75a6656aaa864a041836ffe',
                              'checksum_open': '8239ecd9334a3bc4dfa9a242f7c4d545b08451a1ad468458e20f3d3f768652c3',
                              'checksum_open_type': 'sha256',
                              'checksum_type': 'sha256',
                              'db_version': 10L,
                              'location_href': 'repodata/5b37f89f9f4474801ec5f23dc30d3d6cf9cf663cb75a6656aaa864a041836ffe-filelists.sqlite.bz2',
                              'size': 23038L,
                              'size_open': 200704L,
                              'timestamp': 1355393568L},
             'group': {'checksum': '5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e',
                       'checksum_open': None,
                       'checksum_open_type': None,
                       'checksum_type': 'sha256',
                       'db_version': 0L,
                       'location_href': 'repodata/5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e-comps.xml',
                       'size': 679L,
                       'size_open': 0L,
                       'timestamp': 1355393567L},
             'group_gz': {'checksum': 'c395ae7d8a9117f4e81aa23e37fb9da9865b50917f5f701b50d422875bb0cb14',
                          'checksum_open': '5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e',
                          'checksum_open_type': 'sha256',
                          'checksum_type': 'sha256',
                          'db_version': 0L,
                          'location_href': 'repodata/c395ae7d8a9117f4e81aa23e37fb9da9865b50917f5f701b50d422875bb0cb14-comps.xml.gz',
                          'size': 331L,
                          'size_open': 679L,
                          'timestamp': 1355393567L},
             'origin': {'checksum': 'c949d2b2371fab1a03d03b41057004caf1133a56e4c9236f63b3163ad358c941',
                        'checksum_open': '3928c6aadcfdff101f4482db68c0d07f5777b1c7ad8424e41358bc5e87b8465b',
                        'checksum_open_type': 'sha256',
                        'checksum_type': 'sha256',
                        'db_version': 0L,
                        'location_href': 'repodata/c949d2b2371fab1a03d03b41057004caf1133a56e4c9236f63b3163ad358c941-pkgorigins.gz',
                        'size': 140L,
                        'size_open': 364L,
                        'timestamp': 1355315696L},
             'other': {'checksum': '76b2cfb04531a66e382f187e6a7c90905940d2b2f315b7fd738b839887d83c35',
                       'checksum_open': '2169e09e2c6c91393d38866c501a8697d0a1d698dd3b1027969dc16d291d8915',
                       'checksum_open_type': 'sha256',
                       'checksum_type': 'sha256',
                       'db_version': 0L,
                       'location_href': 'repodata/76b2cfb04531a66e382f187e6a7c90905940d2b2f315b7fd738b839887d83c35-other.xml.gz',
                       'size': 826L,
                       'size_open': 1934L,
                       'timestamp': 1355393567L},
             'other_db': {'checksum': '705a58b0e169bf1d2ade8e4aacc515086644ce16cee971906f920c798c5b17d0',
                          'checksum_open': '916ca5e879387dc1da51b57266bda28a2569d1773ca6c8ea80abe99d9adb373e',
                          'checksum_open_type': 'sha256',
                          'checksum_type': 'sha256',
                          'db_version': 10L,
                          'location_href': 'repodata/705a58b0e169bf1d2ade8e4aacc515086644ce16cee971906f920c798c5b17d0-other.sqlite.bz2',
                          'size': 1462L,
                          'size_open': 8192L,
                          'timestamp': 1355393568L},
             'prestodelta': {'checksum': '26e351e1a38eb1524574e86ab130ea4db780aa1a4a8bb741d37595ed203f931c',
                             'checksum_open': '0052b222add25fed094793c24e73aa07fd598f43f73c1643de26c5e81f6d8c07',
                             'checksum_open_type': 'sha256',
                             'checksum_type': 'sha256',
                             'db_version': 0L,
                             'location_href': 'repodata/26e351e1a38eb1524574e86ab130ea4db780aa1a4a8bb741d37595ed203f931c-prestodelta.xml.gz',
                             'size': 336L,
                             'size_open': 574L,
                             'timestamp': 1337937059L},
             'primary': {'checksum': '5a8e6bbb940b151103b3970a26e32b8965da9e90a798b1b80ee4325308149d8d',
                         'checksum_open': 'b8d60e74c38b94f255c08c3fe5e10c166dcb52f2c4bfec6cae097a68fdd75e74',
                         'checksum_open_type': 'sha256',
                         'checksum_type': 'sha256',
                         'db_version': 0L,
                         'location_href': 'repodata/5a8e6bbb940b151103b3970a26e32b8965da9e90a798b1b80ee4325308149d8d-primary.xml.gz',
                         'size': 956L,
                         'size_open': 3411L,
                         'timestamp': 1355393567L},
             'primary_db': {'checksum': 'a09c42730c03b0d5defa3fd9213794c49e9bafbc67acdd8d4e87a2adf30b8752',
                            'checksum_open': '27b2200efa2c518e5dd5a59deb9ab33c2abca74cb74f5241e612b15931dcec37',
                            'checksum_open_type': 'sha256',
                            'checksum_type': 'sha256',
                            'db_version': 10L,
                            'location_href': 'repodata/a09c42730c03b0d5defa3fd9213794c49e9bafbc67acdd8d4e87a2adf30b8752-primary.sqlite.bz2',
                            'size': 2649L,
                            'size_open': 23552L,
                            'timestamp': 1355393568L},
             'repo_tags': ['test'],
             'revision': '1355393568',
             'updateinfo': {'checksum': '65c4f66e2808d328890505c3c2f13bb35a96f457d1c21a6346191c4dc07e6080',
                            'checksum_open': 'ded9c95e1b88197c906603b5d9693c579cb0afeade3bc7f8ec6cae06b962477d',
                            'checksum_open_type': 'sha256',
                            'checksum_type': 'sha256',
                            'db_version': 0L,
                            'location_href': 'repodata/65c4f66e2808d328890505c3c2f13bb35a96f457d1c21a6346191c4dc07e6080-updateinfo.xml.gz',
                            'size': 55L,
                            'size_open': 42L,
                            'timestamp': 1354188048L}}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_from_bad_url(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.BADURL)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_partial_download_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        print url
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_YUMDLIST, [])
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)

        self.assertEqual(yum_repo,
            {#'deltainfo': None,
             'destdir': self.tmpdir,
             #'filelists': None,
             #'filelists_db': None,
             #'group': None,
             #'group_gz': None,
             #'origin': None,
             #'other': None,
             #'other_db': None,
             #'prestodelta': None,
             #'primary': None,
             #'primary_db': None,
             'repomd': self.tmpdir+'/repodata/repomd.xml',
             #'updateinfo': None,
             'url': url}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_partial_download_repo_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        print url
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_YUMDLIST, ["other", "primary"])
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)

        self.assertEqual(yum_repo,
            {#'deltainfo': None,
             'destdir': self.tmpdir,
             #'filelists': None,
             #'filelists_db': None,
             #'group': None,
             #'group_gz': None,
             #'origin': None,
             'other': self.tmpdir+'/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
             #'other_db': None,
             #'prestodelta': None,
             'primary': self.tmpdir+'/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
             #'primary_db': None,
             'repomd': self.tmpdir+'/repodata/repomd.xml',
             #'updateinfo': None,
             'url': url}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_with_checksum_check(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_corrupted_repo_01_with_checksum_check(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (MOCKURL, config.HARMCHECKSUM % "primary.xml", config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_with_missing_file(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (MOCKURL, config.MISSINGFILE % "primary.xml", config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue(os.path.getsize(yum_repo["primary"]) == 0)

    def test_download_repo_01_with_missing_unwanted_file(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (MOCKURL, config.MISSINGFILE % "primary.xml", config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_YUMDLIST, ["other"])
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_bad_mirrorlist_url(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.BADURL)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

# Metalink tests

    def test_download_repo_01_via_metalink_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_GOOD_01)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:5000/yum/static/01/")
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_metalink_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_GOOD_02)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_metalink_badfilename(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_BADFILENAME)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # All values shoud be None, [], {} or equivalent
        for key in yum_repo.iterkeys():
            self.assertFalse(yum_repo[key])
        for key in yum_repomd.iterkeys():
            self.assertFalse(yum_repomd[key])

    def test_download_repo_01_via_metalink_badchecksum(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_BADCHECKSUM)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # All values shoud be None, [], {} or equivalent
        for key in yum_repo.iterkeys():
            self.assertFalse(yum_repo[key])
        for key in yum_repomd.iterkeys():
            self.assertFalse(yum_repomd[key])

    def test_download_repo_01_via_metalink_nourls(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_NOURLS)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # All values shoud be None, [], {} or equivalent
        for key in yum_repo.iterkeys():
            self.assertFalse(yum_repo[key])
        for key in yum_repomd.iterkeys():
            self.assertFalse(yum_repomd[key])

    def test_download_repo_01_via_metalink_badfirsturl(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_BADFIRSTURL)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:5000/yum/static/01/")

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_metalink_firsturlhascorruptedfiles(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.METALINK_FIRSTURLHASCORRUPTEDFILES)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:5000/yum/harm_checksum/primary.xml/static/01/")

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

# Mirrorlist tests

    def test_download_repo_01_via_mirrorlist_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_GOOD_01)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:5000/yum/static/01/")
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_mirrorlist_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_GOOD_02)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:5000/yum/static/01/")
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_mirrorlist_nourls(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_NOURLS)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # All values shoud be None, [], {} or equivalent
        for key in yum_repo.iterkeys():
            self.assertFalse(yum_repo[key])
        for key in yum_repomd.iterkeys():
            self.assertFalse(yum_repomd[key])

    def test_download_repo_01_via_mirrorlist_badfirsturl(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_BADFIRSTURL)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:5000/yum/static/01/")

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_mirrorlist_firsturlhascorruptedfiles(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:5000/yum/harm_checksum/primary.xml/static/01/")

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

# Update test

    def test_download_and_update_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.setopt(librepo.LRO_MIRRORLIST, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.setopt(librepo.LRO_YUMDLIST, [None])
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:5000/yum/harm_checksum/primary.xml/static/01/")

        # Test that only repomd.xml has a path
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        self.assertTrue(os.path.exists(yum_repo["repomd"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("repomd", "url", "destdir")):
                self.assertTrue(yum_repo[key] == None)

        # Update repo
        h.setopt(librepo.LRO_UPDATE, True)
        h.setopt(librepo.LRO_YUMDLIST, ["primary"])
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Test that only repomd.xml and primary has a path in yum_repo
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        self.assertTrue(os.path.exists(yum_repo["repomd"]))
        self.assertTrue(os.path.exists(yum_repo["primary"]))
        for key in yum_repo.iterkeys():
            if yum_repo[key] and (key not in ("repomd", "primary", "url", "destdir")):
                self.assertTrue(yum_repo[key] == None)

# Base Auth test

    def test_download_repo_01_from_base_auth_secured_web_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (MOCKURL, config.AUTHBASIC, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_download_repo_01_from_base_auth_secured_web_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (MOCKURL, config.AUTHBASIC, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.setopt(librepo.LRO_HTTPAUTH, True)
        h.setopt(librepo.LRO_USERPWD, "%s:%s" % (config.AUTH_USER, config.AUTH_PASS))
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

# Progressbar test

    def test_download_repo_01_with_progressbar(self):
        data = {"ttd": 0, "d": 0, "calls": 0}
        def cb(data, total_to_download, downloaded):
            data["ttd"] = total_to_download
            data["d"] = downloaded
            data["calls"] = data["calls"] + 1

        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (MOCKURL, config.REPO_YUM_01_PATH)
        h.setopt(librepo.LRO_URL, url)
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_DESTDIR, self.tmpdir)
        h.setopt(librepo.LRO_CHECKSUM, True)
        h.setopt(librepo.LRO_PROGRESSDATA, data)
        h.setopt(librepo.LRO_PROGRESSCB, cb)
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue(data["calls"] > 0)
        self.assertTrue(data["ttd"] == data["d"])

