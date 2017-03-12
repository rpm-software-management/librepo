import sys
import time
import gpgme
import shutil
import os.path
import tempfile
import unittest

import librepo

from tests.base import TestCaseWithFlask, TEST_DATA
from tests.servermock.server import app
import tests.servermock.yum_mock.config as config

PUB_KEY = TEST_DATA+"/key.pub"

class TestCaseYumRepoDownloading(TestCaseWithFlask):

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

    def test_download_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)
        timestamp  = r.getinfo(librepo.LRR_YUM_TIMESTAMP)

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
              'url': url,
              'signature': None,
              'mirrorlist': None,
              'metalink': None}
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
                    'db_version': 0,
                    'location_href': 'repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                    'size': 43310,
                    'size_open': 735088,
                    'timestamp': 1347459930},
                'filelists_db': {
                    'checksum': '4034dcea76c94d3f7a9616779539a4ea8cac288f',
                    'checksum_open': '949c6b7b605b2bc66852630c841a5003603ca5b2',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10,
                    'location_href': 'repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                    'size': 22575,
                    'size_open': 201728,
                    'timestamp': 1347459931},
                #'group': None,
                #'group_gz': None,
                #'origin': None,
                'other': {
                    'checksum': 'a8977cdaa0b14321d9acfab81ce8a85e869eee32',
                    'checksum_open': '4b5b8874fb233a626b03b3260a1aa08dce90e81a',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 0,
                    'location_href': 'repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                    'size': 807,
                    'size_open': 1910,
                    'timestamp': 1347459930},
                'other_db': {
                    'checksum': 'fd96942c919628895187778633001cff61e872b8',
                    'checksum_open': 'c5262f62b6b3360722b9b2fb5d0a9335d0a51112',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10,
                    'location_href': 'repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                    'size': 1407,
                    'size_open': 8192,
                    'timestamp': 1347459931},
                #'prestodelta': None,
                'primary': {
                    'checksum': '4543ad62e4d86337cd1949346f9aec976b847b58',
                    'checksum_open': '68457ceb8e20bda004d46e0a4dfa4a69ce71db48',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 0,
                    'location_href': 'repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                    'size': 936,
                    'size_open': 3385,
                    'timestamp': 1347459930},
                'primary_db': {
                    'checksum': '735cd6294df08bdf28e2ba113915ca05a151118e',
                    'checksum_open': 'ba636386312e1b597fc4feb182d04c059b2a77d5',
                    'checksum_open_type': 'sha1',
                    'checksum_type': 'sha1',
                    'db_version': 10,
                    'location_href': 'repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                    'size': 2603,
                    'size_open': 23552,
                    'timestamp': 1347459931},
                'repo_tags': [],
                'revision': '1347459931',
                #'updateinfo': None
                }
        )

        self.assertEqual(timestamp, 1347459931)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_download_repo_01_rpmmd(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_RPMMDREPO
        h.destdir = self.tmpdir
        h.perform(r)

        rpmmd_repo   = r.getinfo(librepo.LRR_RPMMD_REPO)
        rpmmd_repomd = r.getinfo(librepo.LRR_RPMMD_REPOMD)
        timestamp  = r.getinfo(librepo.LRR_RPMMD_TIMESTAMP)

        self.assertEqual(rpmmd_repo,
            { 'destdir': self.tmpdir,
              'url': url,
              'signature': None,
              'mirrorlist': None,
              'metalink': None,
              'repomd': self.tmpdir+'/repodata/repomd.xml',
              'paths': {
                  'primary': self.tmpdir+'/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                  'primary_db': self.tmpdir+'/repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                  'filelists': self.tmpdir+'/repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                  'filelists_db': self.tmpdir+'/repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                  'other': self.tmpdir+'/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                  'other_db': self.tmpdir+'/repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                  }
            }
        )

        self.assertEqual(rpmmd_repomd,
            {   'revision': '1347459931',
                'content_tags': [],
                'distro_tags': [],
                'repo_tags': [],
                'records': {
                    'primary': {
                        'checksum': '4543ad62e4d86337cd1949346f9aec976b847b58',
                        'checksum_open': '68457ceb8e20bda004d46e0a4dfa4a69ce71db48',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 0,
                        'location_href': 'repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                        'size': 936,
                        'size_open': 3385,
                        'timestamp': 1347459930},
                    'primary_db': {
                        'checksum': '735cd6294df08bdf28e2ba113915ca05a151118e',
                        'checksum_open': 'ba636386312e1b597fc4feb182d04c059b2a77d5',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 10,
                        'location_href': 'repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                        'size': 2603,
                        'size_open': 23552,
                        'timestamp': 1347459931},
                    'filelists': {
                        'checksum': 'aeca08fccd3c1ab831e1df1a62711a44ba1922c9',
                        'checksum_open': '52d30ae3162ca863c63c345ffdb7f0e10c1414a5',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 0,
                        'location_href': 'repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                        'size': 43310,
                        'size_open': 735088,
                        'timestamp': 1347459930},
                    'filelists_db': {
                        'checksum': '4034dcea76c94d3f7a9616779539a4ea8cac288f',
                        'checksum_open': '949c6b7b605b2bc66852630c841a5003603ca5b2',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 10,
                        'location_href': 'repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                        'size': 22575,
                        'size_open': 201728,
                        'timestamp': 1347459931},
                    'other': {
                        'checksum': 'a8977cdaa0b14321d9acfab81ce8a85e869eee32',
                        'checksum_open': '4b5b8874fb233a626b03b3260a1aa08dce90e81a',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 0,
                        'location_href': 'repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                        'size': 807,
                        'size_open': 1910,
                        'timestamp': 1347459930},
                    'other_db': {
                        'checksum': 'fd96942c919628895187778633001cff61e872b8',
                        'checksum_open': 'c5262f62b6b3360722b9b2fb5d0a9335d0a51112',
                        'checksum_open_type': 'sha1',
                        'checksum_type': 'sha1',
                        'db_version': 10,
                        'location_href': 'repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                        'size': 1407,
                        'size_open': 8192,
                        'timestamp': 1347459931},
                    }
                }
        )

        self.assertEqual(timestamp, 1347459931)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(rpmmd_repo["destdir"]))
        for key in rpmmd_repo.get("records", []):
            self.assertTrue(os.path.isfile(rpmmd_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)



    def test_download_repo_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_02_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
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
             'url': url,
             'signature': None,
             'mirrorlist': None,
             'metalink': None}
        )

        self.assertEqual(yum_repomd,
            {'content_tags': ['binary-i386'],
             'deltainfo': {'checksum': '32d3307b672abf7356061912fa3dc9b54071c03a75c671111c1c8daf5ed1eb7e',
                           'checksum_open': '8a35a38aef926fd88f479f03a9a22e1ab7aa8bd1aeaa9d05cd696f101eee2846',
                           'checksum_open_type': 'sha256',
                           'checksum_type': 'sha256',
                           'db_version': 0,
                           'location_href': 'repodata/32d3307b672abf7356061912fa3dc9b54071c03a75c671111c1c8daf5ed1eb7e-deltainfo.xml.gz',
                           'size': 300,
                           'size_open': 492,
                           'timestamp': 1355335029},
             'distro_tags': [('cpe:/o:fedoraproject:fedora:17', 'r')],
             'filelists': {'checksum': '2431efa18b5de6bfddb87da2a526362108226752d46ef3a298cd4bf39ba16b1d',
                           'checksum_open': 'afa4a01d7a692ab8105a39fed5535b5011f0c68de0efbc98f9d6ffea36de85fe',
                           'checksum_open_type': 'sha256',
                           'checksum_type': 'sha256',
                           'db_version': 0,
                           'location_href': 'repodata/2431efa18b5de6bfddb87da2a526362108226752d46ef3a298cd4bf39ba16b1d-filelists.xml.gz',
                           'size': 43338,
                           'size_open': 735112,
                           'timestamp': 1355393567},
             'filelists_db': {'checksum': '5b37f89f9f4474801ec5f23dc30d3d6cf9cf663cb75a6656aaa864a041836ffe',
                              'checksum_open': '8239ecd9334a3bc4dfa9a242f7c4d545b08451a1ad468458e20f3d3f768652c3',
                              'checksum_open_type': 'sha256',
                              'checksum_type': 'sha256',
                              'db_version': 10,
                              'location_href': 'repodata/5b37f89f9f4474801ec5f23dc30d3d6cf9cf663cb75a6656aaa864a041836ffe-filelists.sqlite.bz2',
                              'size': 23038,
                              'size_open': 200704,
                              'timestamp': 1355393568},
             'group': {'checksum': '5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e',
                       'checksum_open': None,
                       'checksum_open_type': None,
                       'checksum_type': 'sha256',
                       'db_version': 0,
                       'location_href': 'repodata/5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e-comps.xml',
                       'size': 679,
                       'size_open': 0,
                       'timestamp': 1355393567},
             'group_gz': {'checksum': 'c395ae7d8a9117f4e81aa23e37fb9da9865b50917f5f701b50d422875bb0cb14',
                          'checksum_open': '5b3b362d644e8fa3b359db57be0ff5de8a08365ce9a59cddc3205244a968231e',
                          'checksum_open_type': 'sha256',
                          'checksum_type': 'sha256',
                          'db_version': 0,
                          'location_href': 'repodata/c395ae7d8a9117f4e81aa23e37fb9da9865b50917f5f701b50d422875bb0cb14-comps.xml.gz',
                          'size': 331,
                          'size_open': 679,
                          'timestamp': 1355393567},
             'origin': {'checksum': 'c949d2b2371fab1a03d03b41057004caf1133a56e4c9236f63b3163ad358c941',
                        'checksum_open': '3928c6aadcfdff101f4482db68c0d07f5777b1c7ad8424e41358bc5e87b8465b',
                        'checksum_open_type': 'sha256',
                        'checksum_type': 'sha256',
                        'db_version': 0,
                        'location_href': 'repodata/c949d2b2371fab1a03d03b41057004caf1133a56e4c9236f63b3163ad358c941-pkgorigins.gz',
                        'size': 140,
                        'size_open': 364,
                        'timestamp': 1355315696},
             'other': {'checksum': '76b2cfb04531a66e382f187e6a7c90905940d2b2f315b7fd738b839887d83c35',
                       'checksum_open': '2169e09e2c6c91393d38866c501a8697d0a1d698dd3b1027969dc16d291d8915',
                       'checksum_open_type': 'sha256',
                       'checksum_type': 'sha256',
                       'db_version': 0,
                       'location_href': 'repodata/76b2cfb04531a66e382f187e6a7c90905940d2b2f315b7fd738b839887d83c35-other.xml.gz',
                       'size': 826,
                       'size_open': 1934,
                       'timestamp': 1355393567},
             'other_db': {'checksum': '705a58b0e169bf1d2ade8e4aacc515086644ce16cee971906f920c798c5b17d0',
                          'checksum_open': '916ca5e879387dc1da51b57266bda28a2569d1773ca6c8ea80abe99d9adb373e',
                          'checksum_open_type': 'sha256',
                          'checksum_type': 'sha256',
                          'db_version': 10,
                          'location_href': 'repodata/705a58b0e169bf1d2ade8e4aacc515086644ce16cee971906f920c798c5b17d0-other.sqlite.bz2',
                          'size': 1462,
                          'size_open': 8192,
                          'timestamp': 1355393568},
             'prestodelta': {'checksum': '26e351e1a38eb1524574e86ab130ea4db780aa1a4a8bb741d37595ed203f931c',
                             'checksum_open': '0052b222add25fed094793c24e73aa07fd598f43f73c1643de26c5e81f6d8c07',
                             'checksum_open_type': 'sha256',
                             'checksum_type': 'sha256',
                             'db_version': 0,
                             'location_href': 'repodata/26e351e1a38eb1524574e86ab130ea4db780aa1a4a8bb741d37595ed203f931c-prestodelta.xml.gz',
                             'size': 336,
                             'size_open': 574,
                             'timestamp': 1337937059},
             'primary': {'checksum': '5a8e6bbb940b151103b3970a26e32b8965da9e90a798b1b80ee4325308149d8d',
                         'checksum_open': 'b8d60e74c38b94f255c08c3fe5e10c166dcb52f2c4bfec6cae097a68fdd75e74',
                         'checksum_open_type': 'sha256',
                         'checksum_type': 'sha256',
                         'db_version': 0,
                         'location_href': 'repodata/5a8e6bbb940b151103b3970a26e32b8965da9e90a798b1b80ee4325308149d8d-primary.xml.gz',
                         'size': 956,
                         'size_open': 3411,
                         'timestamp': 1355393567},
             'primary_db': {'checksum': 'a09c42730c03b0d5defa3fd9213794c49e9bafbc67acdd8d4e87a2adf30b8752',
                            'checksum_open': '27b2200efa2c518e5dd5a59deb9ab33c2abca74cb74f5241e612b15931dcec37',
                            'checksum_open_type': 'sha256',
                            'checksum_type': 'sha256',
                            'db_version': 10,
                            'location_href': 'repodata/a09c42730c03b0d5defa3fd9213794c49e9bafbc67acdd8d4e87a2adf30b8752-primary.sqlite.bz2',
                            'size': 2649,
                            'size_open': 23552,
                            'timestamp': 1355393568},
             'repo_tags': ['test'],
             'revision': '1355393568',
             'updateinfo': {'checksum': '65c4f66e2808d328890505c3c2f13bb35a96f457d1c21a6346191c4dc07e6080',
                            'checksum_open': 'ded9c95e1b88197c906603b5d9693c579cb0afeade3bc7f8ec6cae06b962477d',
                            'checksum_open_type': 'sha256',
                            'checksum_type': 'sha256',
                            'db_version': 0,
                            'location_href': 'repodata/65c4f66e2808d328890505c3c2f13bb35a96f457d1c21a6346191c4dc07e6080-updateinfo.xml.gz',
                            'size': 55,
                            'size_open': 42,
                            'timestamp': 1354188048}}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_download_repo_from_bad_url(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.BADURL)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir

        raised = False
        try:
            h.perform(r)
        except librepo.LibrepoException as err:
            raised = True
            unicode_type = unicode if sys.version_info[0] < 3 else str
            self.assertTrue(isinstance(err.args[1], unicode_type))
            self.assertFalse(h.mirrors)
            self.assertFalse(h.metalink)
        finally:
            self.assertTrue(raised)

    def test_partial_download_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.yumdlist = []
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
             'url': url,
             'signature': None,
             'mirrorlist': None,
             'metalink': None}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_partial_download_repo_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.yumdlist = ["other", "primary"]
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
             'url': url,
             'signature': None,
             'mirrorlist': None,
             'metalink': None}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_partial_download_repo_03(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.yumblist = ["other", "filelists"]
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)

        self.assertEqual(yum_repo,
            { #'deltainfo': None,
              'destdir': self.tmpdir,
              #'filelists': self.tmpdir+'/repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
              'filelists_db': self.tmpdir+'/repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
              #'group': None,
              #'group_gz': None,
              #'origin': None,
              #'other': self.tmpdir+'/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
              'other_db': self.tmpdir+'/repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
              #'prestodelta': None,
              'primary': self.tmpdir+'/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
              'primary_db': self.tmpdir+'/repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
              'repomd': self.tmpdir+'/repodata/repomd.xml',
              #'updateinfo': None,
              'url': url,
              'signature': None,
              'mirrorlist': None,
              'metalink': None}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_partial_download_repo_with_substitution(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.yumdlist = ["foo", "primary"]
        h.yumslist = [("foo", "other")]
        h.perform(r)

        yum_repo = r.getinfo(librepo.LRR_YUM_REPO)

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
             'url': url,
             'signature': None,
             'mirrorlist': None,
             'metalink': None}
        )

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

        self.assertFalse(h.mirrors)
        self.assertFalse(h.metalink)

    def test_download_repo_01_without_result_object(self):
        h = librepo.Handle()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        r = h.perform()

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_with_checksum_check(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_corrupted_repo_01_with_checksum_check(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.HARMCHECKSUM % "primary.xml", config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_repo_with_gpg_check(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.gpgcheck = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue("signature" in yum_repo and yum_repo["signature"])
        self.assertTrue(self.tmpdir+'/repodata/repomd.xml.asc' == yum_repo["signature"] )
        self.assertTrue(os.path.isfile(yum_repo["signature"]))

    def test_download_repo_with_gpg_check_bad_signature(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.BADGPG, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.gpgcheck = True
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue("signature" not in yum_repo or yum_repo["signature"])

    def test_download_repo_01_with_missing_file(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.MISSINGFILE % "primary.xml", config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue(os.path.getsize(yum_repo["primary"]) == 0)

    def test_download_repo_01_with_missing_unwanted_file(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.MISSINGFILE % "primary.xml", config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.yumdlist = ["other"]
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_bad_mirrorlist_url(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.BADURL)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

# Metalink tests

    def test_download_only_metalink(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.perform(r)
        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])

        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:%d/yum/static/01/repodata/repomd.xml' % self.PORT,
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'}
            )

    def test_download_repo_01_via_metalink_with_alternates(self):
        h = librepo.Handle()
        h.metalinkurl = "%s%s" % (self.MOCKURL, config.METALINK_WITH_ALTERNATES)
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        r = h.perform()

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(yum_repomd)
        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink,
            {'timestamp': 1381706941,
             'hashes': [
                 ('md5', 'bad'),
                 ('sha1', 'bad'),
                 ('sha256', 'bad'),
                 ('sha512', 'bad')],
             'size': 4761,
             'urls': [{
                 'url': 'http://127.0.0.1:%d/yum/static/01/repodata/repomd.xml' % self.PORT,
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml',
             'alternates': [{
                 'timestamp': 1347459931,
                 'hashes': [
                    ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                    ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                    ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                    ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')
                    ],
                'size': 2621},
                {
                 'timestamp': 123,
                 'hashes': [
                    ('sha1', 'foobar'),
                    ],
                'size': 456}]
            }
            )

    def test_download_repo_01_via_metalink_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(yum_repomd)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:%d/yum/static/01/repodata/repomd.xml' % self.PORT,
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'}
            )

    def test_download_repo_01_via_metalink_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_02)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_metalink_badfilename(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADFILENAME)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertEqual(yum_repo, None)
        self.assertEqual(yum_repomd, None)

    def test_download_repo_01_via_metalink_badchecksum(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADCHECKSUM)
        h.metalinkurl = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo["metalink"])
        # All other values shoud be None, [], {} or equivalent
        for key in yum_repo:
            if key == "metalink":
                continue
            self.assertFalse(yum_repo[key])
        for key in yum_repomd:
            self.assertFalse(yum_repomd[key])

    def test_download_repo_01_via_metalink_nourls(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_NOURLS)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertEqual(yum_repo, None)
        self.assertEqual(yum_repomd, None)

    def test_download_repo_01_via_metalink_badfirsturl(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADFIRSTURL)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_metalink_badfirsturl_maxmirrortries(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADFIRSTHOST)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.maxmirrortries = 1

        # Because first host is bad and maxmirrortries == 1
        # Download should fail
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_download_repo_01_via_metalink_badfirsthost_fastestmirror(self):
        time.sleep(0.5)
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADFIRSTHOST)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fastestmirror = True
        h.fastestmirrortimeout = 5.0
        h.maxmirrortries = 1

        # First host is bad, but fastestmirror is used and thus
        # working mirror should be added to the first position
        # and download should be successful even if maxmirrortries
        # is equal to 1.
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_metalink_badfirsthost_fastestmirror_with_cache(self):
        time.sleep(0.5)
        h = librepo.Handle()
        r = librepo.Result()

        cache = os.path.join(self.tmpdir, "fastestmirror.cache")
        self.assertFalse(os.path.exists(cache))

        url = "%s%s" % (self.MOCKURL, config.METALINK_BADFIRSTHOST)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fastestmirror = True
        h.fastestmirrortimeout = 5.0
        h.fastestmirrorcache = cache
        h.maxmirrortries = 1

        # First host is bad, but fastestmirror is used and thus
        # working mirror should be added to the first position
        # and download should be successful even if maxmirrortries
        # is equal to 1.
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)
        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(os.path.exists(cache))

        shutil.rmtree(os.path.join(self.tmpdir, "repodata"))

        # Try again, this time, fastestmirror cache should be used
        h.perform()

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)
        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(os.path.exists(cache))

    def test_download_repo_01_via_metalink_firsturlhascorruptedfiles(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_FIRSTURLHASCORRUPTEDFILES)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:%d/yum/harm_checksum/primary.xml/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_with_baseurl_and_metalink_specified_only_fetchmirrors(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:%d/yum/static/01/repodata/repomd.xml' % self.PORT,
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'}
            )

    def test_download_repo_01_with_baseurl_and_metalink_specified(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        url = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fastestmirror = True # XXX
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink,
            {'timestamp': 1347459931,
             'hashes': [
                 ('md5', 'f76409f67a84bcd516131d5cc98e57e1'),
                 ('sha1', '75125e73304c21945257d9041a908d0d01d2ca16'),
                 ('sha256', 'bef5d33dc68f47adc7b31df448851b1e9e6bae27840f28700fff144881482a6a'),
                 ('sha512', 'e40060c747895562e945a68967a04d1279e4bd8507413681f83c322479aa564027fdf3962c2d875089bfcb9317d3a623465f390dc1f4acef294711168b807af0')],
             'size': 2621,
             'urls': [{
                 'url': 'http://127.0.0.1:%d/yum/static/01/repodata/repomd.xml' % self.PORT,
                 'type': 'http',
                 'protocol': 'http',
                 'location': 'CZ',
                 'preference': 100}],
             'filename': 'repomd.xml'}
            )

# Mirrorlist tests

    def test_download_only_mirrorlist(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink, None)

    def test_download_repo_01_via_mirrorlist_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_mirrorlist_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_02)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)
        self.assertTrue(yum_repomd)

    def test_download_repo_01_via_mirrorlist_nourls(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_NOURLS)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertEqual(yum_repo, None)
        self.assertEqual(yum_repomd, None)

    def test_download_repo_01_via_mirrorlist_badfirsturl(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_BADFIRSTURL)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"], "http://127.0.0.1:%d/yum/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_mirrorlist_firsturlhascorruptedfiles(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:%d/yum/harm_checksum/primary.xml/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_via_mirrorlist_firsturlhascorruptedfiles_maxmirrortries_enabled(self):
        """Download should fails on the first mirror (one file has a bad checksum).
        Other mirrors have the file with a good checksum, but option
        LRO_MAXMIRRORTRIES should prevent trying of other mirrors."""
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.maxmirrortries = 1
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_download_repo_01_via_mirrorlist_firsturlhascorruptedfiles_maxmirrortries_enabled_2(self):
        """Download should fails on the first mirror (one file has a bad checksum).
        Other mirrors have the file with a good checksum.
        LRO_MAXMIRRORTRIES should allow try one next mirror. Thus repo
        should be downloaded without error."""
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.maxmirrortries = 2
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:%d/yum/harm_checksum/primary.xml/static/01/" % self.PORT)

        # Test if all mentioned files really exist
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("url", "destdir")):
                self.assertTrue(os.path.isfile(yum_repo[key]))

    def test_download_repo_01_with_baseurl_and_mirrorlist_specified_only_fetchmirrors(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.perform(r)

        self.assertEqual(h.mirrors,
                         ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink, None)

    def test_download_repo_01_with_baseurl_and_mirrorlist_specified(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.perform(r)

        self.assertEqual(h.mirrors,
                         ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink, None)

# Update test

    def test_download_and_update_repo_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_FIRSTURLHASCORRUPTEDFILES)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.yumdlist = [None]
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertEqual(yum_repo["url"],
            "http://127.0.0.1:%d/yum/harm_checksum/primary.xml/static/01/" % self.PORT)

        # Test that only repomd.xml has a path
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        self.assertTrue(os.path.exists(yum_repo["repomd"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("repomd", "url", "destdir", "mirrorlist")):
                self.assertTrue(yum_repo[key] == None)

        # Update repo
        h.update = True
        h.yumdlist = ["primary"]
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        # Test that only repomd.xml and primary has a path in yum_repo
        self.assertTrue(os.path.isdir(yum_repo["destdir"]))
        self.assertTrue(os.path.exists(yum_repo["repomd"]))
        self.assertTrue(os.path.exists(yum_repo["primary"]))
        for key in yum_repo:
            if yum_repo[key] and (key not in ("repomd", "primary", "url", "destdir", "mirrorlist")):
                self.assertTrue(yum_repo[key] == None)

# Base Auth test

    def test_download_repo_01_from_base_auth_secured_web_01(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.AUTHBASIC, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_download_repo_01_from_base_auth_secured_web_02(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s%s" % (self.MOCKURL, config.AUTHBASIC, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.httpauth = True
        h.userpwd = "%s:%s" % (config.AUTH_USER, config.AUTH_PASS)
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

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.progressdata = data
        h.progresscb = cb
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue(data["calls"] > 0)
        self.assertTrue(data["ttd"] == data["d"] or data["ttd"] == 0)

# Var substitution test

    def test_download_repo_01_with_url_substitution(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH_VAR)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.checksum = True
        h.varsub = config.REPO_YUM_01_VARSUB_LIST
        h.perform(r)

        yum_repo   = r.getinfo(librepo.LRR_YUM_REPO)
        yum_repomd = r.getinfo(librepo.LRR_YUM_REPOMD)

        self.assertTrue(yum_repo)
        self.assertTrue(yum_repomd)
        self.assertTrue(yum_repo["url"].endswith(config.REPO_YUM_01_PATH))

    def test_download_repo_01_mirrorlist_substitution(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_VARED)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.varsub = config.MIRRORLIST_VARED_LIST
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink, None)

    def test_download_repo_01_mirrorlist_with_url_substitution(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.MIRRORLIST_VARSUB)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.varsub = config.MIRRORLIST_VARSUB_LIST
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink, None)

    def test_download_repo_01_metalink_with_url_substitution(self):
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.METALINK_VARSUB)
        h.mirrorlist = url
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.fetchmirrors = True
        h.varsub = config.METALINK_VARSUB_LIST
        h.perform(r)

        self.assertEqual(h.mirrors, ['http://127.0.0.1:%d/yum/static/01/' % self.PORT])
        self.assertEqual(h.metalink["urls"],
            [{
                'url': 'http://127.0.0.1:%d/yum/static/$version/repodata/repomd.xml' % self.PORT,
                'type': 'http',
                'protocol': 'http',
                'location': 'CZ',
                'preference': 100}
            ])

    def test_download_with_gpgcheck_enabled_but_without_signature(self):
        # At first, download whole repository
        h = librepo.Handle()
        r = librepo.Result()

        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_02_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        h.gpgcheck = True
        self.assertRaises(librepo.LibrepoException, h.perform, (r))

    def test_download_with_custom_http_headers(self):
        h = librepo.Handle()
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h.urls = [url]
        h.repotype = librepo.LR_YUMREPO
        h.destdir = self.tmpdir
        headers = ["Accept: audio/mpeg"]
        h.httpheader = headers
        del headers
        h.perform()

    def test_download_with_local_enabled(self):
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)

        # Make local cache
        h_l = librepo.Handle()
        h_l.repotype = librepo.YUMREPO
        h_l.urls = [url]
        h_l.destdir = self.tmpdir
        h_l.perform()

        self.assertFalse(h_l.mirrors)  # No mirrors should be listed

        # Create a handle for the local cache
        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.urls = [self.tmpdir]
        h.metalinkurl = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        h.local = True
        h.perform()

        self.assertFalse(h.mirrors)  # Remote metalik is specified
                                     # No mirrors should be listed

        # Disable local
        h.local = False

        # Try to download something (don't care that it doesn't exist)
        librepo.download_packages([librepo.PackageTarget("Foo", handle=h)])

        self.assertTrue(h.mirrors)  # List of mirrors should be re-initialized
                                    # and should contain mirrors from the metalink

    def test_download_with_offline_enabled_01(self):
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.urls = [url]
        h.destdir = self.tmpdir
        h.offline = True
        # Only one remote URL is specified - this should fail
        self.assertRaises(librepo.LibrepoException, h.perform)

        shutil.rmtree(os.path.join(self.tmpdir, "repodata"))

        # But everything should be ok when we disable the option
        h.offline = False
        h.perform()

    def test_download_with_offline_enabled_02(self):
        url = "%s%s" % (self.MOCKURL, config.REPO_YUM_01_PATH)
        dir_01 = os.path.join(self.tmpdir, "01")
        dir_02 = os.path.join(self.tmpdir, "02")
        os.makedirs(dir_01)
        os.makedirs(dir_02)

        # Prepare local cache
        h_l = librepo.Handle()
        h_l.repotype = librepo.YUMREPO
        h_l.urls = [url]
        h_l.destdir = dir_01
        h_l.perform()

        # Let's work offline
        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.urls = ["http://foo.bar/xyz", dir_01]
        h.metalinkurl = "http://foo.bar/mtl"
        h.mirrorlisturl = "http://foo.bar/mrl"
        h.fastestmirror = True
        h.destdir = dir_02
        h.offline = True
        # Bad URLs for metalink and mirrorlist should be ok, they
        # should be ignored - we should work offline
        # First remote URL set in urls option should be ignored
        # Fastestmirror should be skipped
        h.perform()

    def test_download_with_offline_enabled_03(self):
        url_mtl = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        url_mrl = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)

        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.metalinkurl = url_mtl
        h.mirrorlisturl = url_mrl
        h.fastestmirror = True
        h.offline = True
        h.fetchmirrors = True
        h.destdir = self.tmpdir
        # We only want to fetch mirrors
        # But both metalink and mirrorlist is specified by remote URL
        # Output mirrorlist should be empty
        h.perform()
        self.assertFalse(h.mirrors)

        # But when we disable offline, it should work
        h.offline = False
        h.perform()
        self.assertTrue(h.mirrors)

    def test_download_with_offline_enabled_04(self):
        url_mtl = "%s%s" % (self.MOCKURL, config.METALINK_GOOD_01)
        url_mrl = "%s%s" % (self.MOCKURL, config.MIRRORLIST_GOOD_01)
        dir_01 = os.path.join(self.tmpdir, "01")
        dir_02 = os.path.join(self.tmpdir, "02")
        os.makedirs(dir_01)
        os.makedirs(dir_02)

        EXP_MRS = [u'http://127.0.0.1:%s/yum/static/01/' % self.PORT,
                   u'http://127.0.0.1:%s/yum/static/01/' % self.PORT]

        # 1) At first, work online
        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.metalinkurl = url_mtl
        h.mirrorlisturl = url_mrl
        h.fastestmirror = True
        h.offline = False
        h.destdir = dir_01
        h.perform()
        self.assertTrue(h.mirrors)

        # 2) Then switch to offline mode
        h.offline = True
        h.destdir = dir_02
        # We should not be able to download the repodata
        self.assertRaises(librepo.LibrepoException, h.perform)
        # Mirrors should still be there from the first try
        self.assertTrue(h.mirrors)
        self.assertEqual(h.mirrors, EXP_MRS)

        # 3) Let's stay offline but use repodata from first try
        shutil.rmtree(os.path.join(dir_02, "repodata"))
        h.urls = [dir_01]
        h.perform()
        # Mirrors should still be there from the first try
        self.assertTrue(h.mirrors)
        self.assertEqual(h.mirrors, EXP_MRS)

        # 4) Still offline, but mirrors get reset
        shutil.rmtree(os.path.join(dir_02, "repodata"))
        h.metalinkurl = url_mtl
        h.mirrorlisturl = url_mrl
        h.perform()
        # Mirrors should be reset and thus empty
        self.assertFalse(h.mirrors)

        # 5) Create a brand new handle and load a local mirrorlists
        shutil.rmtree(os.path.join(dir_02, "repodata"))
        h = librepo.Handle()
        h.repotype = librepo.YUMREPO
        h.metalinkurl = os.path.join(dir_01, "metalink.xml")
        h.mirrorlisturl = os.path.join(dir_01, "mirrorlist")
        h.fastestmirror = True
        h.offline = True
        h.destdir = dir_02
        # Perform should fail, because we are working offline
        self.assertRaises(librepo.LibrepoException, h.perform)
        # The local mirrorlists should be loaded
        self.assertTrue(h.mirrors)
        self.assertEqual(h.mirrors, EXP_MRS)


