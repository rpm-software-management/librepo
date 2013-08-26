import unittest
import os
import time
import librepo

from tests.base import TestCase, TEST_DATA

REPO_YUM_01_PATH = TEST_DATA+"/repo_yum_01/"

class TestCaseYum(unittest.TestCase):
    def test_yum_repomd_get_age(self):
        h = librepo.Handle()

        h.setopt(librepo.LRO_URLS, [REPO_YUM_01_PATH])
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
        h.setopt(librepo.LRO_LOCAL, True)

        r = h.perform()

        repomd_path = r.getinfo(librepo.LRR_YUM_REPO)["repomd"]
        mtime = os.stat(repomd_path).st_mtime
        calculated_age = time.time() - mtime

        librepo_age =  librepo.yum_repomd_get_age(r)

        # Approximate comparison
        self.assertTrue(((calculated_age - 10) < librepo_age) or \
                         (calculated_age + 10) > librepo_age)
