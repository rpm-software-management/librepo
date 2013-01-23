import unittest
import librepo

class TestCaseHandle(unittest.TestCase):
    def test_handle_setopt_getinfo(self):
        """No exception shoud be raised."""
        h = librepo.Handle()

        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  True)
        self.assertTrue(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  False)
        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  1)
        self.assertTrue(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  0)
        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))

        self.assertEqual(h.getinfo(librepo.LRI_URL), None)
        h.setopt(librepo.LRO_URL, "http://foo")
        self.assertEqual(h.getinfo(librepo.LRI_URL), "http://foo")
        h.setopt(librepo.LRO_URL, "")
        self.assertEqual(h.getinfo(librepo.LRI_URL), "")
        h.setopt(librepo.LRO_URL, None)
        self.assertEqual(h.getinfo(librepo.LRI_URL), None)

        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), None)
        h.setopt(librepo.LRO_MIRRORLIST, "http://ml")
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), "http://ml")
        h.setopt(librepo.LRO_MIRRORLIST, "")
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), "")
        h.setopt(librepo.LRO_MIRRORLIST, None)
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), None)

        self.assertFalse(h.getinfo(librepo.LRI_LOCAL))
        h.setopt(librepo.LRO_LOCAL,  1)
        self.assertTrue(h.getinfo(librepo.LRI_LOCAL))
        h.setopt(librepo.LRO_LOCAL,  0)
        self.assertFalse(h.getinfo(librepo.LRI_LOCAL))

        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), None)
        h.setopt(librepo.LRO_DESTDIR,  "foodir")
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), "foodir")
        h.setopt(librepo.LRO_DESTDIR,  None)
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), None)
        h.setopt(librepo.LRO_DESTDIR,  "")
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), "")

        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), None)
        h.setopt(librepo.LRO_YUMDLIST,  ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), ["primary", "other"])
        h.setopt(librepo.LRO_YUMDLIST,  [])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), [])
        h.setopt(librepo.LRO_YUMDLIST,  [None])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), [])

        def callback(data, total_to_download, downloaded):
            pass
        h.setopt(librepo.LRO_PROGRESSCB, None)
        h.setopt(librepo.LRO_PROGRESSCB, callback)
        h.setopt(librepo.LRO_PROGRESSCB, None)

