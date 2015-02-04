import unittest
import librepo
from librepo import LibrepoException

class TestCaseResult(unittest.TestCase):

    def test_result_getinfo(self):
        r = librepo.Result()
        self.assertTrue(r)

        self.assertRaises(ValueError, r.getinfo, 99999999)
        self.assertFalse(r.getinfo(librepo.LRR_YUM_REPO))
        self.assertFalse(r.getinfo(librepo.LRR_YUM_REPOMD))
        self.assertRaises(LibrepoException, r.getinfo, librepo.LRR_YUM_TIMESTAMP)

    def test_result_attrs(self):
        r = librepo.Result()
        self.assertTrue(r)

        self.assertRaises(AttributeError, getattr, r, 'foobar_attr')

        # Attrs should not be filled (that's why None or
        # LibrepoException is expected), but they definitelly
        # should exists (not AttributeError should be raised)
        self.assertFalse(r.yum_repo)
        self.assertFalse(r.yum_repomd)
        self.assertRaises(LibrepoException, getattr, r, 'yum_timestamp')
