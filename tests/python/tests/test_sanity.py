import unittest
import librepo

class TestCaseSanity(unittest.TestCase):
    def test_raw_handle_sanity(self):
        h = librepo.Handle()
        self.assertEqual(h.update, False)
        self.assertEqual(h.urls, [])
        self.assertEqual(h.mirrorlist, None)
        self.assertEqual(h.local, False)
        self.assertEqual(h.progresscb, None)
        self.assertEqual(h.progressdata, None)
        self.assertEqual(h.destdir, None)
        self.assertEqual(h.repotype, 0)
        self.assertEqual(h.useragent, None)
        self.assertEqual(h.yumdlist, None)
        self.assertEqual(h.rpmmddlist, None)
        self.assertEqual(h.yumblist, None)
        self.assertEqual(h.rpmmdblist, None)
        self.assertEqual(h.fetchmirrors, False)
        self.assertEqual(h.maxmirrortries, 0)
        self.assertEqual(h.varsub, None)
        self.assertEqual(h.mirrors, [])
        self.assertEqual(h.metalink, None)
        self.assertEqual(h.hmfcb, None)

    def test_raw_result_sanity(self):
        r = librepo.Result()
        self.assertEqual(r.yum_repo, None)
        self.assertEqual(r.yum_repomd, None)
        self.assertEqual(r.rpmmd_repo, None)
        self.assertEqual(r.rpmmd_repomd, None)

    def test_raw_result_sanity(self):
        t = librepo.PackageTarget("foo")
        self.assertEqual(t.handle, None)
        self.assertEqual(t.relative_url, "foo")
        self.assertEqual(t.dest, None)
        self.assertEqual(t.base_url, None)
        self.assertEqual(t.checksum_type, librepo.CHECKSUM_UNKNOWN)
        self.assertEqual(t.checksum, None)
        self.assertEqual(t.progresscb, None)
        self.assertEqual(t.cbdata, None)
        self.assertEqual(t.local_path, None)
        self.assertEqual(t.err, None)
