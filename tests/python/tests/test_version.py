import unittest
import librepo

class Version(unittest.TestCase):
    def test_version(self):
        self.assertIsInstance(librepo.VERSION, unicode)
        self.assertGreaterEqual(len(librepo.VERSION), 5)
        self.assertEqual(librepo.VERSION.count('.'), 2)
